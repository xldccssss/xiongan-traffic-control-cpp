#include "DemandEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <QJsonArray>

#include "RoutingEngine.h"
#include "ILane.h"
#include "Plugin/_datastruct.h"
#include "ilink.h"
#include "ivehicle.h"
#include "netinterface.h"
#include "simuinterface.h"
#include "tessinterface.h"

namespace {
// CPython random.Random integer seeding and _randbelow implementation.  The
// Python reference uses Random(vehicleGeneration + 7919).shuffle(sequence),
// so using QRandomGenerator would preserve only the ratio, not run identity.
class PythonRandom
{
public:
    explicit PythonRandom(quint64 seed)
    {
        std::array<quint32, 2> key{{
            static_cast<quint32>(seed & 0xffffffffULL),
            static_cast<quint32>(seed >> 32)
        }};
        const int keyLength = key[1] == 0 ? 1 : 2;
        seedMt(19650218U);
        int i = 1, j = 0;
        for (int k = qMax(624, keyLength); k > 0; --k) {
            m_state[i] = (m_state[i]
                ^ ((m_state[i - 1] ^ (m_state[i - 1] >> 30)) * 1664525U))
                + key[j] + static_cast<quint32>(j);
            ++i; ++j;
            if (i >= 624) { m_state[0] = m_state[623]; i = 1; }
            if (j >= keyLength) j = 0;
        }
        for (int k = 623; k > 0; --k) {
            m_state[i] = (m_state[i]
                ^ ((m_state[i - 1] ^ (m_state[i - 1] >> 30)) * 1566083941U))
                - static_cast<quint32>(i);
            ++i;
            if (i >= 624) { m_state[0] = m_state[623]; i = 1; }
        }
        m_state[0] = 0x80000000U;
    }

    quint32 below(quint32 n)
    {
        if (n <= 1U) return 0U;
        int bits = 0;
        for (quint32 value = n; value; value >>= 1U) ++bits;
        quint32 value = 0;
        do { value = next() >> (32 - bits); } while (value >= n);
        return value;
    }

private:
    void seedMt(quint32 seed)
    {
        m_state[0] = seed;
        for (int i = 1; i < 624; ++i)
            m_state[i] = 1812433253U
                * (m_state[i - 1] ^ (m_state[i - 1] >> 30))
                + static_cast<quint32>(i);
        m_index = 624;
    }

    quint32 next()
    {
        if (m_index >= 624) {
            static constexpr quint32 mag[2] = {0U, 0x9908b0dfU};
            int kk = 0;
            for (; kk < 227; ++kk) {
                const quint32 y = (m_state[kk] & 0x80000000U)
                    | (m_state[kk + 1] & 0x7fffffffU);
                m_state[kk] = m_state[kk + 397] ^ (y >> 1) ^ mag[y & 1U];
            }
            for (; kk < 623; ++kk) {
                const quint32 y = (m_state[kk] & 0x80000000U)
                    | (m_state[kk + 1] & 0x7fffffffU);
                m_state[kk] = m_state[kk - 227] ^ (y >> 1) ^ mag[y & 1U];
            }
            const quint32 y = (m_state[623] & 0x80000000U)
                | (m_state[0] & 0x7fffffffU);
            m_state[623] = m_state[396] ^ (y >> 1) ^ mag[y & 1U];
            m_index = 0;
        }
        quint32 y = m_state[m_index++];
        y ^= y >> 11;
        y ^= (y << 7) & 0x9d2c5680U;
        y ^= (y << 15) & 0xefc60000U;
        y ^= y >> 18;
        return y;
    }

    std::array<quint32, 624> m_state{};
    int m_index = 624;
};
}

DemandEngine::DemandEngine(const ProjectModel* model)
{
    setModel(model);
}

void DemandEngine::setModel(const ProjectModel* model)
{
    m_model = model;
    if (m_model) {
        const QJsonObject demand = m_model->scenario().value("demand").toObject();
        m_outerRate = demand.value("outerBoundaryVehiclesPerEntryPerHour").toInt(1000);
        m_residentialRate = demand.value("residentialVehiclesPerEntryPerHour").toInt(100);
        m_vehicleSeed = qBound(0, m_model->scenario().value("randomSeeds")
            .toObject().value("vehicleGeneration").toInt(20260822), 2147483647);
        m_preloadLimit = m_model->placements().size();
        buildVehicleTypeCycle();
    } else {
        m_preloadLimit = 0;
    }
    reset();
}

void DemandEngine::setRandomSeed(int seed)
{
    m_vehicleSeed = qBound(0, seed, 2147483647);
    buildVehicleTypeCycle();
}

void DemandEngine::setPreloadLimit(int count)
{
    m_preloadLimit = qBound(0, count, m_model ? m_model->placements().size() : 0);
}

void DemandEngine::reset()
{
    m_preloadCreated = 0;
    m_continuousCreated = 0;
    const int terminalCount=m_model?m_model->terminals().size():0;
    const bool warm=m_model&&m_model->scenario().value("runtimeVehicleGeneration")
        .toObject().value("warmStartOneVehiclePerEntry").toBool(true);
    m_terminalCredit.fill(warm?1.0:0.0,terminalCount);
    m_nextRetrySec.fill(0.0,terminalCount);
    m_lastTerminalReleaseSec.fill(-1e9, terminalCount);
    m_pendingPlacements.clear();
    m_placementAttempts.fill(0,m_model?m_model->placements().size():0);
    for(int i=0;m_model&&i<preloadTarget();++i)m_pendingPlacements.enqueue(i);
    m_laneCursorByLink.clear();m_knownVehicleIds.clear();m_terminalRotation=0;
    m_vehicleTypeCursor=0;m_vehicleNameSequence=0;m_lastGenerationSec=-1.0;m_nowSec=0.0;
}

void DemandEngine::buildVehicleTypeCycle()
{
    m_vehicleTypeCycle.clear();if(!m_model)return;
    const QJsonObject generation=m_model->scenario().value("runtimeVehicleGeneration").toObject();
    QJsonArray types=generation.value("vehicleTypes").toArray();
    if(types.isEmpty())types=m_model->scenario().value("initialTraffic").toObject().value("vehicleTypes").toArray();
    const int target=qMax(10,generation.value("vehicleTypeCycleSize").toInt(100));
    struct Allocation{SpawnVehicleType type;double exact=0.0;int count=0;int order=0;};
    QVector<Allocation> values;double total=0.0;
    for(int i=0;i<types.size();++i){const QJsonObject item=types[i].toObject();Allocation a;
        a.type.code=item.value("vehicleTypeCode").toInt(1);
        a.type.speedFactor=qMax(0.1,item.value("initialSpeedFactor").toDouble(1.0));
        a.type.color=item.value("color").toString("#3D8BFF");a.exact=qMax(0.0,item.value("share").toDouble());a.order=i;total+=a.exact;values.push_back(a);}
    if(values.isEmpty()){m_vehicleTypeCycle.fill(SpawnVehicleType(),target);return;}
    int assigned=0;for(Allocation&a:values){a.exact=target*a.exact/qMax(1e-9,total);a.count=static_cast<int>(a.exact);assigned+=a.count;}
    QVector<int> order(values.size());for(int i=0;i<order.size();++i)order[i]=i;
    std::sort(order.begin(),order.end(),[&](int a,int b){const double fa=values[a].exact-values[a].count,fb=values[b].exact-values[b].count;return fa!=fb?fa>fb:values[a].order<values[b].order;});
    for(int i=0;i<target-assigned;++i)++values[order[i%order.size()]].count;
    for(const Allocation&a:values)for(int i=0;i<a.count;++i)m_vehicleTypeCycle.push_back(a.type);
    const quint64 seed=static_cast<quint64>(m_vehicleSeed)+7919ULL;
    PythonRandom random(seed);
    for(int i=m_vehicleTypeCycle.size()-1;i>0;--i){
        const int j=static_cast<int>(random.below(static_cast<quint32>(i+1)));
        m_vehicleTypeCycle.swapItemsAt(i,j);
    }
}

QVector<int> DemandEngine::vehicleTypeCycleCodesForTest() const
{
    QVector<int> result;
    result.reserve(m_vehicleTypeCycle.size());
    for (const SpawnVehicleType& type : m_vehicleTypeCycle) result.push_back(type.code);
    return result;
}

int DemandEngine::preloadTarget() const
{
    return m_model ? qBound(0, m_preloadLimit, m_model->placements().size()) : 0;
}

bool DemandEngine::createPlacement(const InitialPlacement& placement,
                                   SimuInterface* simulation,
                                   RoutingEngine* routing)
{
    Online::DynaVehiParam parameter;
    parameter.vehiTypeCode = placement.vehicleTypeCode;
    parameter.roadId = placement.linkId;
    parameter.laneNumber = placement.laneNumber;
    parameter.dist = placement.distanceM;
    parameter.speed = placement.speedMps;
    parameter.color = placement.color;
    parameter.name = QStringLiteral("INIT_%1_%2").arg(placement.vehicleTypeKey)
        .arg(placement.placementId,4,10,QLatin1Char('0')).left(31);
    IVehicle* vehicle = simulation->createGVehicle(parameter);
    if (!vehicle||vehicle->id()<=0||m_knownVehicleIds.contains(vehicle->id())) return false;
    int actualOrigin = -1;
    int destination = -1;
    for (int i = 0; i < m_model->terminals().size(); ++i) {
        if (m_model->terminals()[i].id == placement.originTerminalId) actualOrigin = i;
        if (m_model->terminals()[i].id == placement.destinationTerminalId) destination = i;
    }
    if (!routing->registerVehicle(vehicle, actualOrigin, destination,
                                  placement.destinationExitLinkId, false)
        || !routing->primeVehicleRoute(vehicle, placement.linkId, true)) {
        routing->unregisterVehicle(vehicle->id());
        simulation->stopVehicleDriving(vehicle);
        return false;
    }
    m_knownVehicleIds.insert(vehicle->id());
    return true;
}

bool DemandEngine::createAtTerminal(int terminalIndex,
                                    SimuInterface* simulation,
                                    RoutingEngine* routing)
{
    if (!m_model || terminalIndex < 0 || terminalIndex >= m_model->terminals().size()) return false;
    const TerminalDefinition& terminal = m_model->terminals()[terminalIndex];
    ILink* link = gpTessInterface->netInterface()->findLink(terminal.entryLinkId);
    if (!link || link->laneCount() <= 0) return false;
    QList<ILane*> lanes;
    const QSet<qint64> blocked=m_model->blockedSpawnLanesByLink().value(terminal.entryLinkId);
    for(ILane* lane:link->lanes())if(lane&&!blocked.contains(lane->id()))lanes.push_back(lane);
    if(lanes.isEmpty())return false;
    // Bind the fixed OD before choosing the physical spawn lane. Round-robin
    // over every entry lane used to create vehicles in a lane that could not
    // serve their first route movement, forcing an immediate lane change next
    // to the first boundary signal and blocking both lanes.
    const int destinationTerminal = routing->nextDestinationForOrigin(terminalIndex);
    if (destinationTerminal < 0
        || destinationTerminal >= m_model->terminals().size()) return false;
    const qint64 destinationExitLinkId =
        m_model->terminals()[destinationTerminal].exitLinkId;
    const qint64 firstTargetLinkId = routing->nextHopFor(
        destinationExitLinkId, terminal.entryLinkId);
    const QSet<qint64> compatibleLaneIds = m_model->movementLanesByLink()
        .value(terminal.entryLinkId).value(firstTargetLinkId);
    if (!compatibleLaneIds.isEmpty()) {
        QList<ILane*> compatible;
        for (ILane* lane : lanes)
            if (lane && compatibleLaneIds.contains(lane->id()))
                compatible.push_back(lane);
        if (!compatible.isEmpty()) lanes = compatible;
    }
    const int cursor=m_laneCursorByLink.value(terminal.entryLinkId,0);
    ILane* selectedLane=lanes[cursor%lanes.size()];
    m_laneCursorByLink[terminal.entryLinkId]=cursor+1;
    if(m_vehicleTypeCycle.isEmpty())buildVehicleTypeCycle();
    const SpawnVehicleType type=m_vehicleTypeCycle.value(
        m_vehicleTypeCursor%qMax(1,m_vehicleTypeCycle.size()),SpawnVehicleType());
    const QJsonObject generation=m_model->scenario().value("runtimeVehicleGeneration").toObject();
    Online::DynaVehiParam parameter;
    parameter.vehiTypeCode = type.code;
    parameter.roadId = terminal.entryLinkId;
    parameter.laneNumber = selectedLane->number();
    parameter.dist = generation.value("spawnDistanceM").toDouble(0.0);
    parameter.speed = generation.value("initialSpeedMps").toDouble(5.0)*type.speedFactor;
    parameter.color = type.color;
    parameter.name=QStringLiteral("CAV_%1_%2").arg(terminal.id.left(8))
        .arg(++m_vehicleNameSequence,10,10,QLatin1Char('0')).left(31);
    IVehicle* vehicle = simulation->createGVehicle(parameter);
    if (!vehicle||vehicle->id()<=0||m_knownVehicleIds.contains(vehicle->id())) return false;
    if (!routing->registerVehicle(vehicle, terminalIndex, destinationTerminal,
                                  destinationExitLinkId)
        || !routing->primeVehicleRoute(vehicle, terminal.entryLinkId)) {
        routing->unregisterVehicle(vehicle->id());
        simulation->stopVehicleDriving(vehicle);
        return false;
    }
    m_knownVehicleIds.insert(vehicle->id());
    ++m_vehicleTypeCursor;
    ++m_continuousCreated;
    return true;
}

void DemandEngine::step(double simulationTimeSec, double deltaSec,
                        SimuInterface* simulation, RoutingEngine* routing)
{
    if (!m_model || !simulation || !routing) return;
    const double preloadWindowSec = m_model->scenario().value("initialTraffic")
        .toObject().value("preloadWindowSec").toDouble(15.0);
    const int maximumCreates = m_model->scenario().value("initialTraffic")
        .toObject().value("maximumCreatesPerStep").toInt(44);
    int attempts = 0;
    while (simulationTimeSec <= preloadWindowSec
           && !m_pendingPlacements.isEmpty()
           && attempts < maximumCreates) {
        const int index=m_pendingPlacements.dequeue();
        const InitialPlacement& placement = m_model->placements()[index];
        if (createPlacement(placement, simulation, routing)) ++m_preloadCreated;
        else {const int maximumAttempts=m_model->scenario().value("initialTraffic").toObject()
                .value("maximumAttemptsPerVehicle").toInt(3);
            if(++m_placementAttempts[index]<maximumAttempts)m_pendingPlacements.enqueue(index);}
        ++attempts;
    }
    if (simulationTimeSec < preloadWindowSec) return;
    const double elapsed=m_lastGenerationSec<0.0?0.0:qMin(10.0,qMax(0.0,simulationTimeSec-m_lastGenerationSec));
    m_lastGenerationSec=simulationTimeSec;m_nowSec=simulationTimeSec;
    for(int i=0;i<m_model->terminals().size();++i){const int rate=m_model->terminals()[i].group=="RESIDENTIAL_AREA"?m_residentialRate:m_outerRate;m_terminalCredit[i]+=rate*elapsed/3600.0;}
    int createdThisStep = 0;
    const int n=m_model->terminals().size();const int start=n?m_terminalRotation%n:0;
    if(n)m_terminalRotation=(m_terminalRotation+1)%n;
    const double retry=qMax(0.05,m_model->scenario().value("runtimeVehicleGeneration")
        .toObject().value("retryIntervalSec").toDouble(0.5));
    const double minimumReleaseHeadway = qMax(0.1,
        m_model->scenario().value("runtimeVehicleGeneration").toObject()
            .value("minimumSuccessfulSpawnHeadwaySec").toDouble(1.5));
    for (int offset = 0; offset < n; ++offset) {
        const int i=(start+offset)%n;if(simulationTimeSec+1e-9<m_nextRetrySec[i])continue;
        if (simulationTimeSec + 1e-9
            < m_lastTerminalReleaseSec.value(i, -1e9) + minimumReleaseHeadway)
            continue;
        int due=qMin(static_cast<int>(std::floor(m_terminalCredit[i])),maximumCreates-createdThisStep);
        while (due > 0 && createdThisStep < maximumCreates) {
            if (createAtTerminal(i, simulation, routing)){
                m_terminalCredit[i]=qMax(0.0,m_terminalCredit[i]-1.0);
                m_lastTerminalReleaseSec[i]=simulationTimeSec;
                --due;
                ++createdThisStep;
                // Do not flush a congestion backlog into one boundary point
                // in the same 100 ms batch.  The credit remains queued and is
                // discharged later at a physically plausible headway.
                break;
            }
            else {m_nextRetrySec[i]=simulationTimeSec+retry;break;}
        }
        if(createdThisStep>=maximumCreates)break;
    }
    Q_UNUSED(deltaSec);
}
