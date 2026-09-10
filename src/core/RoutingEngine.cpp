#include "RoutingEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include "IDecisionPoint.h"
#include "ILane.h"
#include "ILaneConnector.h"
#include "IRouting.h"
#include "SignalEngine.h"
#include <QCryptographicHash>
#include <QMutexLocker>
#include "ilink.h"
#include "ivehicle.h"
#include "ivehicledriving.h"
#include "netinterface.h"
#include "tessinterface.h"

RoutingEngine::RoutingEngine(const ProjectModel* model)
{
    setModel(model);
}

void RoutingEngine::setDestinationSeed(int seed)
{
    QMutexLocker locker(&m_mutex);
    m_destinationSeed = static_cast<quint32>(qBound(0, seed, 2147483647));
}

void RoutingEngine::setSimulationTime(double nowSec)
{
    QMutexLocker locker(&m_mutex);
    m_nowSec = nowSec;
}

int RoutingEngine::vehicleCountWithOd() const
{
    QMutexLocker locker(&m_mutex);
    return m_vehicleOd.size();
}

qint64 RoutingEngine::routeDecisionCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_routeDecisions;
}

qint64 RoutingEngine::routeChangeCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_routeChanges;
}

qint64 RoutingEngine::nativeRouteSyncCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_nativeRouteSyncs;
}

qint64 RoutingEngine::routeFallbackCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_routeFallbacks;
}

VehicleOdState RoutingEngine::vehicleOdState(qint64 vehicleId) const
{
    QMutexLocker locker(&m_mutex);
    return m_vehicleOd.value(vehicleId);
}

QHash<qint64, VehicleOdState> RoutingEngine::vehicleOdSnapshot() const
{
    QMutexLocker locker(&m_mutex);
    return m_vehicleOd;
}

double RoutingEngine::edgeCostFor(qint64 fromLinkId, qint64 toLinkId) const
{
    QMutexLocker locker(&m_mutex);
    return m_edgeCost.value(fromLinkId).value(toLinkId, -1.0);
}

qint64 RoutingEngine::nextHopFor(qint64 destinationLinkId,
                                 qint64 currentLinkId) const
{
    QMutexLocker locker(&m_mutex);
    return m_nextHopByDestination.value(destinationLinkId)
        .value(currentLinkId, 0);
}

QVector<qint64> RoutingEngine::plannedPathForTest(
    qint64 startLinkId, qint64 destinationLinkId) const
{
    QMutexLocker locker(&m_mutex);
    return buildExecutablePath(startLinkId, destinationLinkId,
                               QVector<qint64>{startLinkId});
}

void RoutingEngine::setModel(const ProjectModel* model)
{
    QMutexLocker locker(&m_mutex);
    if (m_model != model) m_executableRoutingByPath.clear();
    m_model = model;
    if (m_model) {
        m_config = m_model->scenario().value("cav").toObject();
        m_destinationSeed = static_cast<quint32>(m_model->scenario()
            .value("randomSeeds").toObject().value("destinationChoice")
            .toInt(20260823));
    }
    reset();
}

void RoutingEngine::reset()
{
    QMutexLocker locker(&m_mutex);
    m_vehicleOd.clear();
    m_decisionPointByLink.clear();
    m_nextHopByDestination.clear();
    m_distanceByDestination.clear();
    m_freeFlowDistanceByDestination.clear();
    m_edgeCost.clear();
    m_reservations.clear();
    m_originDepartureOrdinal.fill(0, m_model ? m_model->terminals().size() : 0);
    m_routeDecisions = 0;
    m_routeChanges = 0;
    m_nativeRouteSyncs = 0;
    m_routeFallbacks = 0;
    m_nowSec = 0.0;
}

double RoutingEngine::linkCost(qint64 linkId,
                               const QHash<qint64, LinkTraffic>& traffic) const
{
    const GraphLink link = m_model->links().value(linkId);
    const LinkTraffic observed = traffic.value(linkId);
    const double minimumSpeed = qMax(0.1,
        m_config.value("minimumObservedSpeedMps").toDouble(1.5));
    const double freeFlow = qMax(1.0, link.freeSpeedMps);
    const double baseTime = qMax(0.1, link.lengthM) / freeFlow;
    const double speed = observed.vehicleCount > 0
        ? qMax(minimumSpeed, observed.meanSpeedMps) : freeFlow;
    double cost = observed.vehicleCount > 0
        ? qMax(baseTime, qMax(0.1, link.lengthM) / speed) : baseTime;
    cost += observed.queueCount
        * m_config.value("queuePenaltySecPerVehicle").toDouble(1.2);
    cost += observed.occupancy * observed.occupancy
        * m_config.value("occupancyPenaltySec").toDouble(25.0);
    if (observed.occupancy >= m_config.value("spillbackThreshold").toDouble(0.82))
        cost += m_config.value("spillbackPenaltySec").toDouble(120.0);
    return cost;
}

void RoutingEngine::rebuildShortestPathForests(
    const TrafficAggregator& aggregate,
    const SignalEngine* signalEngine,double nowSec)
{
    QMutexLocker locker(&m_mutex);
    if (!m_model) return;
    const QHash<qint64, LinkTraffic>& traffic=aggregate.links();
    const QHash<qint64, LaneTraffic>& lanes=aggregate.lanes();
    m_nowSec = nowSec;
    QHash<qint64, QVector<GraphEdge>> reverse;
    for (const GraphEdge& edge : m_model->edges()) reverse[edge.to].push_back(edge);
    m_nextHopByDestination.clear();
    m_distanceByDestination.clear();
    m_edgeCost.clear();
    m_reservations.clear();
    const double signalWeight=m_config.value("signalDelayWeight").toDouble(0.8);
    const double movementQueueWeight=m_config.value("movementQueuePenaltySecPerVehicle").toDouble(3.0);
    const double movementOccupancyWeight=m_config.value("movementOccupancyPenaltySec").toDouble(18.0);
    const double movementSpeedWeight=m_config.value("movementSpeedDeficitPenaltySec").toDouble(0.5);
    for(const GraphEdge& edge:m_model->edges()){
        const GraphLink from=m_model->links().value(edge.from),to=m_model->links().value(edge.to);
        const double connectorSpeed=qMax(1.0,qMin(from.freeSpeedMps,to.freeSpeedMps));
        double movementCost=0.0;
        const QSet<qint64> movementLanes=m_model->movementLanesByLink()
            .value(edge.from).value(edge.to);
        if(!movementLanes.isEmpty()){
            double count=0.0,speedSum=0.0,queueSum=0.0;
            double bestQueue=std::numeric_limits<double>::infinity(),occupancy=0.0;
            for(qint64 laneId:movementLanes){const LaneTraffic lane=lanes.value(laneId);
                count+=lane.vehicleCount;speedSum+=lane.meanSpeedMps*lane.vehicleCount;
                queueSum+=lane.queueCount;bestQueue=qMin(bestQueue,static_cast<double>(lane.queueCount));occupancy+=lane.occupancy;}
            const double laneCount=movementLanes.size();
            const double meanSpeed=count>0.0?speedSum/count:qMax(1.0,from.freeSpeedMps);
            const double meanQueue=queueSum/laneCount;
            occupancy/=laneCount;
            const double speedDeficit=qMax(0.0,(qMax(1.0,from.freeSpeedMps)
                -qMax(m_config.value("minimumObservedSpeedMps").toDouble(1.5),meanSpeed))
                /qMax(1.0,from.freeSpeedMps));
            movementCost=movementQueueWeight*(bestQueue+0.35*meanQueue)
                +movementOccupancyWeight*occupancy*occupancy
                +movementSpeedWeight*speedDeficit;
        }
        const double wait=signalEngine?signalEngine->waitForMovement(edge.from,edge.to,nowSec):0.0;
        // Prefer capacity-preserving movements when several OD-valid paths
        // exist. The congestion term is deliberately dynamic: an empty
        // one-lane street remains usable, but it becomes much less attractive
        // than a wider parallel corridor as its occupancy rises. A lane drop
        // also receives a fixed merge-friction charge, preventing a wide
        // approach from continuously feeding a narrow downstream bottleneck.
        const int movementLaneCount = movementLanes.isEmpty()
            ? qMin(qMax(1, from.laneCount), qMax(1, to.laneCount))
            : movementLanes.size();
        const int usableLaneCount = qMax(1,
            qMin(qMax(1, to.laneCount), movementLaneCount));
        const int lostLanes = qMax(0, qMax(1, from.laneCount) - usableLaneCount);
        const double downstreamOccupancy = qBound(0.0,
            traffic.value(edge.to).occupancy, 1.0);
        const double laneDropPenalty = qMax(0.0,
            m_config.value("laneDropPenaltySecPerLane").toDouble(20.0))
            * lostLanes;
        const double narrowCongestionPenalty = qMax(0.0,
            m_config.value("narrowLinkCongestionPenaltySec").toDouble(30.0))
            * downstreamOccupancy * downstreamOccupancy / usableLaneCount;
        const double movementScarcityPenalty = qMax(0.0,
            m_config.value("movementLaneScarcityPenaltySec").toDouble(4.0))
            / usableLaneCount;
        m_edgeCost[edge.from][edge.to]=qMax(0.1,edge.connectorLengthM)/connectorSpeed
            +linkCost(edge.to,traffic)+signalWeight*wait+movementCost
            +laneDropPenalty+narrowCongestionPenalty+movementScarcityPenalty;
    }

    using QueueItem = std::pair<double, qint64>;
    for (const TerminalDefinition& terminal : m_model->terminals()) {
        const qint64 destination = terminal.exitLinkId;
        QHash<qint64, double> distance;
        QHash<qint64, qint64> nextHop;
        std::priority_queue<QueueItem, std::vector<QueueItem>, std::greater<QueueItem>> queue;
        distance.insert(destination, 0.0);
        queue.push({0.0, destination});
        while (!queue.empty()) {
            const QueueItem current = queue.top();
            queue.pop();
            if (current.first > distance.value(current.second,
                    std::numeric_limits<double>::infinity()) + 1e-9) continue;
            for (const GraphEdge& edge : reverse.value(current.second)) {
                const double candidate = current.first
                    + m_edgeCost.value(edge.from).value(edge.to,
                        std::numeric_limits<double>::infinity());
                if (candidate + 1e-9 < distance.value(
                        edge.from, std::numeric_limits<double>::infinity())) {
                    distance.insert(edge.from, candidate);
                    nextHop.insert(edge.from, edge.to);
                    queue.push({candidate, edge.from});
                }
            }
        }
        m_nextHopByDestination.insert(destination, nextHop);
        m_distanceByDestination.insert(destination, distance);
        if(!m_freeFlowDistanceByDestination.contains(destination)){
            QHash<qint64,double> freeDistance;freeDistance.insert(destination,0.0);
            std::priority_queue<QueueItem,std::vector<QueueItem>,std::greater<QueueItem>> freeQueue;
            freeQueue.push({0.0,destination});
            while(!freeQueue.empty()){const QueueItem current=freeQueue.top();freeQueue.pop();
                if(current.first>freeDistance.value(current.second,std::numeric_limits<double>::infinity())+1e-9)continue;
                for(const GraphEdge& edge:reverse.value(current.second)){
                    const GraphLink from=m_model->links().value(edge.from),to=m_model->links().value(edge.to);
                    const double edgeFree=qMax(0.1,edge.connectorLengthM)/qMax(1.0,qMin(from.freeSpeedMps,to.freeSpeedMps))
                        +qMax(0.1,to.lengthM)/qMax(1.0,to.freeSpeedMps);
                    const double candidate=current.first+edgeFree;
                    if(candidate+1e-9<freeDistance.value(edge.from,std::numeric_limits<double>::infinity())){freeDistance[edge.from]=candidate;freeQueue.push({candidate,edge.from});}}
            }
            m_freeFlowDistanceByDestination.insert(destination,freeDistance);
        }
    }
}

int RoutingEngine::seededIndex(quint32 seed,int modulo,const QStringList& identity)const
{
    if(modulo<=0)return -1;
    QStringList parts;parts<<QString::number(seed);parts.append(identity);
    const QByteArray digest=QCryptographicHash::hash(parts.join('|').toUtf8(),QCryptographicHash::Sha256);
    quint64 value=0;for(int i=0;i<8;++i)value=(value<<8)|static_cast<unsigned char>(digest[i]);
    return static_cast<int>(value%static_cast<quint64>(modulo));
}

int RoutingEngine::deterministicDestination(int originTerminal,qint64 ordinal)const
{
    if(!m_model||m_model->terminals().size()<2||originTerminal<0)return -1;
    QVector<int> options;for(int i=0;i<m_model->terminals().size();++i)if(i!=originTerminal)options.push_back(i);
    const int index=seededIndex(m_destinationSeed,options.size(),
        {QStringLiteral("RUNTIME_OD"),m_model->terminals()[originTerminal].id,QString::number(ordinal)});
    return options.value(index,-1);
}

int RoutingEngine::nextDestinationForOrigin(int originTerminal) const
{
    QMutexLocker locker(&m_mutex);
    return deterministicDestination(
        originTerminal, m_originDepartureOrdinal.value(originTerminal, 0));
}

bool RoutingEngine::registerVehicle(IVehicle* vehicle, int originTerminal,
                                    int destinationTerminal,
                                    qint64 fixedDestinationExitLinkId,
                                    bool advanceOriginOrdinal)
{
    QMutexLocker locker(&m_mutex);
    if (!vehicle || !m_model) return false;
    if (m_vehicleOd.contains(vehicle->id())) return true;
    if (originTerminal < 0 && vehicle->startLink())
        originTerminal = m_model->terminalIndexByEntryLink().value(
            vehicle->startLink()->id(), -1);
    if (destinationTerminal < 0 && fixedDestinationExitLinkId > 0)
        destinationTerminal = m_model->terminalIndexByExitLink().value(
            fixedDestinationExitLinkId, -1);
    const qint64 ordinal=m_originDepartureOrdinal.value(originTerminal,0);
    if (destinationTerminal < 0 || destinationTerminal == originTerminal)
        destinationTerminal = deterministicDestination(originTerminal,ordinal);
    if (originTerminal < 0 || destinationTerminal < 0) return false;
    VehicleOdState state;
    state.originTerminal = originTerminal;
    state.destinationTerminal = destinationTerminal;
    state.originEntryLinkId = originTerminal>=0&&originTerminal<m_model->terminals().size()
        ?m_model->terminals()[originTerminal].entryLinkId:0;
    state.isInitialVehicle = !advanceOriginOrdinal;
    state.departureSec = m_nowSec;
    state.destinationExitLinkId = fixedDestinationExitLinkId > 0
        ? fixedDestinationExitLinkId
        : m_model->terminals()[destinationTerminal].exitLinkId;
    m_vehicleOd.insert(vehicle->id(), state);
    if(advanceOriginOrdinal&&originTerminal>=0&&originTerminal<m_originDepartureOrdinal.size())
        ++m_originDepartureOrdinal[originTerminal];
    vehicle->setHookedUp(true);
    vehicle->setJsonProperty(QStringLiteral("isCAV"), true);
    vehicle->setJsonProperty(QStringLiteral("isInitialVehicle"), state.isInitialVehicle);
    vehicle->setJsonProperty(QStringLiteral("controlStack"),
                             QStringLiteral("local-terminal-rsu-cav"));
    vehicle->setJsonProperty(QStringLiteral("originTerminalId"),
                             m_model->terminals()[originTerminal].id);
    vehicle->setJsonProperty(QStringLiteral("destinationTerminalId"),
                             m_model->terminals()[destinationTerminal].id);
    vehicle->setJsonProperty(QStringLiteral("destinationExitLinkId"),
                             state.destinationExitLinkId);
    vehicle->setJsonProperty(QStringLiteral("routingMode"),
                             QStringLiteral("CAV_DYNAMIC_DESTINATION_PRESERVING"));
    return true;
}

bool RoutingEngine::primeVehicleRoute(IVehicle* vehicle, qint64 currentLinkId,
                                      bool constrainToCurrentLane)
{
    QMutexLocker locker(&m_mutex);
    if (!vehicle || !m_model || !m_vehicleOd.contains(vehicle->id())) return false;
    if (!m_model->decisionsByLink().contains(currentLinkId)) return true;
    if (m_decisionPointByLink.isEmpty() && gpTessInterface
        && gpTessInterface->netInterface()) {
        for (IDecisionPoint* point : gpTessInterface->netInterface()->decisionPoints())
            if (point && point->link()) m_decisionPointByLink.insert(point->link()->id(), point);
    }
    IDecisionPoint* point = m_decisionPointByLink.value(currentLinkId, nullptr);
    if (!point) return false;
    if (constrainToCurrentLane && vehicle->lane()) {
        QVector<qint64> reachable;
        for (ILaneConnector* connector : vehicle->lane()->toLaneConnectors()) {
            if (!connector || !connector->toLane()
                || !connector->toLane()->link())
                continue;
            const qint64 target = connector->toLane()->link()->id();
            if (!reachable.contains(target)) reachable.push_back(target);
        }
        const qint64 selected = chooseNext(vehicle->id(), currentLinkId, reachable);
        if (selected > 0
            && synchronizeNativeRoute(vehicle, currentLinkId, selected)) {
            ++m_routeDecisions;
            return true;
        }
    }
    applyDecision(vehicle, point, nullptr);
    const VehicleOdState state = m_vehicleOd.value(vehicle->id());
    if (state.lockedDecisionLinkId != currentLinkId || state.lockedNextLinkId <= 0)
        return false;
    vehicle->setJsonProperty(QStringLiteral("nextLinkId"), state.lockedNextLinkId);
    return true;
}

bool RoutingEngine::ensureVehicleRoute(IVehicle* vehicle, qint64 currentLinkId)
{
    QMutexLocker locker(&m_mutex);
    if (!vehicle || !m_model || currentLinkId <= 0) return false;
    registerVehicle(vehicle);
    if (!m_vehicleOd.contains(vehicle->id())) return false;
    const qint64 owner = m_model->decisionOwnerByCorridorLink()
        .value(currentLinkId, 0);
    if (owner <= 0) return true;

    const VehicleOdState& current = m_vehicleOd.value(vehicle->id());
    if (current.lockedDecisionLinkId == owner) {
        if (currentLinkId == owner && current.lockedNextLinkId > 0)
            return true;
        for (qint64 routeLinkId : current.lockedRouteLinkIds)
            if (routeLinkId == currentLinkId)
                return true;
        if (m_model->movementLanesByLink().value(currentLinkId)
                .contains(current.lockedNextLinkId))
            return true;
    }

    auto lockLocalNextHop = [&]() {
        const VehicleOdState& od = m_vehicleOd.value(vehicle->id());
        const qint64 next = m_nextHopByDestination
            .value(od.destinationExitLinkId).value(currentLinkId, 0);
        if (next <= 0 || isForbiddenDestinationTarget(od, next)) {
            ++m_routeFallbacks;
            return false;
        }
        if (!synchronizeNativeRoute(vehicle, currentLinkId, next)) {
            ++m_routeFallbacks;
            return false;
        }
        ++m_routeDecisions;
        vehicle->setJsonProperty(QStringLiteral("nextLinkId"), next);
        vehicle->setJsonProperty(QStringLiteral("routingExecution"),
            QStringLiteral("CPP_COMPLETE_OD_ROUTE_FALLBACK"));
        return true;
    };

    if (m_decisionPointByLink.isEmpty() && gpTessInterface
        && gpTessInterface->netInterface()) {
        for (IDecisionPoint* point
             : gpTessInterface->netInterface()->decisionPoints())
            if (point && point->link())
                m_decisionPointByLink.insert(point->link()->id(), point);
    }
    IDecisionPoint* point = m_decisionPointByLink.value(owner, nullptr);
    if (!point) return lockLocalNextHop();

    if (currentLinkId != owner) {
        const LocalDecision decision=m_model->decisionsByLink().value(owner);
        QVector<qint64> routeTargets;
        for(const LocalRouteOption&option:decision.routes)
            if(!routeTargets.contains(option.nextLinkId))
                routeTargets.push_back(option.nextLinkId);
        const qint64 selected=chooseNext(vehicle->id(),
            decision.routingContextLinkId,routeTargets);
        if(selected>0&&synchronizeNativeRoute(vehicle,currentLinkId,selected)){
            ++m_routeDecisions;
            return true;
        }
        return lockLocalNextHop();
    }

    // afterDistriRouting is still the primary SDK path.  This call is the
    // deterministic in-process fallback for batches where TESSNG does not
    // emit that callback for a vehicle entering a relocated decision corridor.
    applyDecision(vehicle, point, vehicle->routing());
    const VehicleOdState updated = m_vehicleOd.value(vehicle->id());
    if (updated.lockedDecisionLinkId == owner
        && updated.lockedNextLinkId > 0) return true;
    return lockLocalNextHop();
}

double RoutingEngine::freeFlowTravelTimeSec(qint64 startLinkId,qint64 destinationLinkId)const
{
    QMutexLocker locker(&m_mutex);
    if(!m_model||!m_model->links().contains(startLinkId))return -1.0;
    const double remaining=m_freeFlowDistanceByDestination.value(destinationLinkId)
        .value(startLinkId,std::numeric_limits<double>::infinity());
    if(!std::isfinite(remaining))return -1.0;
    const GraphLink source=m_model->links().value(startLinkId);
    return remaining+qMax(0.1,source.lengthM)/qMax(1.0,source.freeSpeedMps);
}

void RoutingEngine::unregisterVehicle(qint64 vehicleId)
{
    QMutexLocker locker(&m_mutex);
    m_vehicleOd.remove(vehicleId);
}

qint64 RoutingEngine::movementTargetLink(qint64 vehicleId,
                                         qint64 currentLinkId) const
{
    QMutexLocker locker(&m_mutex);
    if (!m_model || !m_vehicleOd.contains(vehicleId)) return 0;
    const VehicleOdState& od = m_vehicleOd.value(vehicleId);
    if (currentLinkId <= 0) return od.lockedNextLinkId;

    // The complete route assigned to TESSNG is authoritative between decision
    // points. Using the newly rebuilt shortest-path tree here made the lane
    // controller chase a different turn every five seconds while the native
    // vehicle still followed the old route. Near a terminal this caused late
    // forced lane changes, empty connector filters, and wrong-exit holds.
    const QHash<qint64, QSet<qint64>> movements =
        m_model->movementLanesByLink().value(currentLinkId);
    for (int index = 0; index + 1 < od.lockedRouteLinkIds.size(); ++index) {
        if (od.lockedRouteLinkIds[index] != currentLinkId) continue;
        const qint64 next = od.lockedRouteLinkIds[index + 1];
        if (movements.isEmpty() || movements.contains(next)) return next;
        break;
    }
    // lockedNextLinkId belongs to the most recent decision. After a vehicle
    // leaves that corridor it can be stale, so use it only when it remains a
    // real outgoing movement of the current link.
    if (movements.contains(od.lockedNextLinkId))
        return od.lockedNextLinkId;
    const qint64 next = m_nextHopByDestination.value(od.destinationExitLinkId)
        .value(currentLinkId, 0);
    return movements.isEmpty() || movements.contains(next) ? next : 0;
}

qint64 RoutingEngine::recoverReachableMovementTarget(
    qint64 vehicleId, qint64 currentLinkId,
    const QVector<qint64>& reachableTargets)
{
    QMutexLocker locker(&m_mutex);
    if (!m_vehicleOd.contains(vehicleId) || reachableTargets.isEmpty())
        return 0;
    const VehicleOdState odSnapshot = m_vehicleOd.value(vehicleId);
    QVector<qint64> safeTargets;
    safeTargets.reserve(reachableTargets.size());
    for (qint64 target : reachableTargets)
        if (!isForbiddenDestinationTarget(odSnapshot, target)
            && !safeTargets.contains(target))
            safeTargets.push_back(target);
    if (safeTargets.isEmpty()) return 0;
    const qint64 current = movementTargetLink(vehicleId, currentLinkId);
    if (safeTargets.contains(current)) return current;

    // A lane-change may not finish before the stop-line freeze zone.  Select
    // the best connector that is physically reachable from the current lane,
    // while still minimizing the remaining cost to the vehicle's fixed OD.
    const qint64 selected = chooseNext(vehicleId, currentLinkId, safeTargets);
    if (selected <= 0) return 0;
    VehicleOdState& od = m_vehicleOd[vehicleId];
    if (od.lockedNextLinkId != selected) {
        od.lockedNextLinkId = selected;
        od.lastDecisionContextLinkId = currentLinkId;
        od.lastDecisionAtSec = m_nowSec;
        ++od.routeChanges;
        ++m_routeChanges;
    }
    return selected;
}

QVector<qint64> RoutingEngine::buildExecutablePath(
    qint64 currentLinkId, qint64 destinationLinkId,
    const QVector<qint64>& routePrefix) const
{
    QVector<qint64> path;
    if (!m_model || currentLinkId <= 0 || destinationLinkId <= 0
        || routePrefix.isEmpty())
        return path;

    const int currentIndex = routePrefix.indexOf(currentLinkId);
    if (currentIndex < 0) return path;
    QSet<qint64> visited;
    for (int index = currentIndex; index < routePrefix.size(); ++index) {
        const qint64 linkId = routePrefix[index];
        if (!m_model->links().contains(linkId) || visited.contains(linkId))
            return QVector<qint64>();
        if (!path.isEmpty()
            && !m_edgeCost.value(path.last()).contains(linkId))
            return QVector<qint64>();
        if (linkId != destinationLinkId
            && (m_model->terminalIndexByExitLink().contains(linkId)
                || m_model->forbiddenExitLinkIds().contains(linkId)))
            return QVector<qint64>();
        path.push_back(linkId);
        visited.insert(linkId);
    }

    const int maximumLinks = qMax(1, m_model->links().size());
    while (!path.isEmpty() && path.last() != destinationLinkId
           && path.size() <= maximumLinks) {
        const qint64 next = m_nextHopByDestination
            .value(destinationLinkId).value(path.last(), 0);
        if (next <= 0 || !m_edgeCost.value(path.last()).contains(next)
            || visited.contains(next))
            return QVector<qint64>();
        if (next != destinationLinkId
            && (m_model->terminalIndexByExitLink().contains(next)
                || m_model->forbiddenExitLinkIds().contains(next)))
            return QVector<qint64>();
        path.push_back(next);
        visited.insert(next);
    }
    if (path.isEmpty() || path.last() != destinationLinkId)
        return QVector<qint64>();
    return path;
}

bool RoutingEngine::applyExecutableRouting(
    IVehicle* vehicle, qint64 currentLinkId,
    const QVector<qint64>& routePrefix, QVector<qint64>* appliedLinkIds)
{
    if (!vehicle || !vehicle->vehicleDriving() || !m_model
        || !m_vehicleOd.contains(vehicle->id()))
        return false;
    const qint64 destination = m_vehicleOd.value(vehicle->id())
        .destinationExitLinkId;
    const QVector<qint64> path = buildExecutablePath(
        currentLinkId, destination, routePrefix);
    if (path.size() < 2) return false;

    IRouting* currentRouting = vehicle->routing();
    if (currentRouting) {
        QVector<qint64> currentPath;
        const QList<ILink*> links = currentRouting->getLinks();
        int index = -1;
        for (int i = 0; i < links.size(); ++i)
            if (links[i] && links[i]->id() == currentLinkId) {
                index = i;
                break;
            }
        for (int i = index; i >= 0 && i < links.size(); ++i)
            if (links[i]) currentPath.push_back(links[i]->id());
        if (currentPath == path) {
            if (appliedLinkIds) *appliedLinkIds = path;
            return true;
        }
    }

    QStringList keyParts;
    QList<ILink*> links;
    keyParts.reserve(path.size());
    links.reserve(path.size());
    NetInterface* net = gpTessInterface ? gpTessInterface->netInterface()
                                        : nullptr;
    if (!net) return false;
    for (qint64 linkId : path) {
        ILink* link = net->findLink(linkId);
        if (!link) return false;
        links.push_back(link);
        keyParts.push_back(QString::number(linkId));
    }
    const QString key = keyParts.join(QLatin1Char(','));
    IRouting* routing = m_executableRoutingByPath.value(key, nullptr);
    if (!routing) {
        routing = net->createRouting(links);
        if (!routing) return false;
        m_executableRoutingByPath.insert(key, routing);
    }
    if (!vehicle->vehicleDriving()->setRouting(routing)) return false;
    if (appliedLinkIds) *appliedLinkIds = path;
    return true;
}

qint64 RoutingEngine::nativeMovementTarget(IVehicle* vehicle,
                                           qint64 currentLinkId) const
{
    if (!vehicle || currentLinkId <= 0) return 0;
    IRouting* routing = vehicle->routing();
    if (!routing) return 0;
    const QList<ILink*> links = routing->getLinks();
    for (int index = 0; index + 1 < links.size(); ++index)
        if (links[index] && links[index + 1]
            && links[index]->id() == currentLinkId)
            return links[index + 1]->id();
    return 0;
}

bool RoutingEngine::synchronizeNativeRoute(IVehicle* vehicle,
                                           qint64 currentLinkId,
                                           qint64 reachableTargetLinkId)
{
    QMutexLocker locker(&m_mutex);
    if (!vehicle || !vehicle->vehicleDriving() || !m_model
        || !m_vehicleOd.contains(vehicle->id())
        || currentLinkId <= 0 || reachableTargetLinkId <= 0
        || isForbiddenDestinationTarget(
            m_vehicleOd.value(vehicle->id()), reachableTargetLinkId))
        return false;
    if (m_decisionPointByLink.isEmpty() && gpTessInterface
        && gpTessInterface->netInterface()) {
        for (IDecisionPoint* point : gpTessInterface->netInterface()->decisionPoints())
            if (point && point->link())
                m_decisionPointByLink.insert(point->link()->id(), point);
    }

    const qint64 decisionLinkId = m_model->decisionOwnerByCorridorLink()
        .value(currentLinkId, currentLinkId);
    IDecisionPoint* point = m_decisionPointByLink.value(decisionLinkId, nullptr);
    QVector<qint64> prefix;
    if (point) {
        for (IRouting* routing : point->routings()) {
            if (!routing) continue;
            const QList<ILink*> links = routing->getLinks();
            bool containsCurrent = false;
            for (ILink* link : links)
                if (link && link->id() == currentLinkId) {
                    containsCurrent = true;
                    break;
                }
            if (!containsCurrent || links.isEmpty() || !links.last()
                || links.last()->id() != reachableTargetLinkId)
                continue;
            for (ILink* link : links)
                if (link) prefix.push_back(link->id());
            break;
        }
    }
    // On a relocated decision corridor the reachable lane connector can be
    // an intermediate link rather than the final link of the local routing.
    // A direct physical graph edge is still a valid prefix for a complete
    // fixed-OD path.
    if (prefix.isEmpty()
        && m_edgeCost.value(currentLinkId).contains(reachableTargetLinkId))
        prefix = {currentLinkId, reachableTargetLinkId};
    QVector<qint64> executablePath;
    if (prefix.isEmpty()
        || !applyExecutableRouting(vehicle, currentLinkId, prefix,
                                   &executablePath))
        return false;

    VehicleOdState& od = m_vehicleOd[vehicle->id()];
    od.lockedDecisionLinkId = decisionLinkId;
    od.lockedNextLinkId = reachableTargetLinkId;
    od.lastDecisionContextLinkId = currentLinkId;
    od.lastDecisionAtSec = m_nowSec;
    od.lockedRouteLinkIds = executablePath;
    // recoverReachableMovementTarget already accounts for this route change.
    // Updating TESSNG's native routing object is one execution repair, not a
    // second route decision.
    ++m_nativeRouteSyncs;
    vehicle->setJsonProperty(QStringLiteral("nextLinkId"),
                             reachableTargetLinkId);
    return true;
}

qint64 RoutingEngine::chooseNext(qint64 vehicleId, qint64 currentLinkId,
                                 const QVector<qint64>& available)
{
    QMutexLocker locker(&m_mutex);
    if (!m_vehicleOd.contains(vehicleId) || available.isEmpty()) return 0;
    VehicleOdState& od = m_vehicleOd[vehicleId];
    const double repeat=qMax(0.0,m_config.value("minimumDecisionRepeatSec").toDouble(3.0));
    const bool lock=m_config.value("lockDecisionUntilIncomingLinkExit").toBool(true);
    if (od.lastDecisionContextLinkId == currentLinkId
        && (lock||m_nowSec-od.lastDecisionAtSec<repeat)
        && available.contains(od.lockedNextLinkId))
        return od.lockedNextLinkId;
    const double reservationPenalty=m_config.value("cooperativeReservationPenaltySec").toDouble(2.5);
    qint64 best=0;double bestScore=std::numeric_limits<double>::infinity();
    QVector<qint64> sorted=available;std::sort(sorted.begin(),sorted.end());
    for(qint64 target:sorted){const double edge=m_edgeCost.value(currentLinkId).value(target,
            std::numeric_limits<double>::infinity());
        const double remaining=m_distanceByDestination.value(od.destinationExitLinkId).value(target,
            std::numeric_limits<double>::infinity());if(!std::isfinite(edge)||!std::isfinite(remaining))continue;
        const QString key=QStringLiteral("%1:%2:%3").arg(currentLinkId).arg(target).arg(od.destinationExitLinkId);
        const int reserved=m_reservations.value(key,0);
        const quint64 tie=((static_cast<quint64>(vehicleId)*1103515245ULL
            +static_cast<quint64>(target)*2654435761ULL)&0xffffULL);
        const double score=edge+remaining+reservationPenalty*reserved+tie*1e-9;
        if(score<bestScore){bestScore=score;best=target;}}
    // A relocated decision can use an upstream context link that has no
    // direct edge to the physically reachable connector target. In that case
    // fall back to the remaining fixed-OD cost, never to an arbitrary turn.
    if (best <= 0) {
        for (qint64 target : sorted) {
            double remaining = m_distanceByDestination
                .value(od.destinationExitLinkId)
                .value(target, std::numeric_limits<double>::infinity());
            if (!std::isfinite(remaining))
                remaining = m_freeFlowDistanceByDestination
                    .value(od.destinationExitLinkId)
                    .value(target, std::numeric_limits<double>::infinity());
            if (!std::isfinite(remaining)) continue;
            const GraphLink link = m_model->links().value(target);
            const double entryCost = qMax(0.1, link.lengthM)
                / qMax(1.0, link.freeSpeedMps);
            const quint64 tie = ((static_cast<quint64>(vehicleId) * 1103515245ULL
                + static_cast<quint64>(target) * 2654435761ULL) & 0xffffULL);
            const double score = entryCost + remaining + tie * 1e-9;
            if (score < bestScore) { bestScore = score; best = target; }
        }
        ++m_routeFallbacks;
    }
    if(best>0){const QString key=QStringLiteral("%1:%2:%3").arg(currentLinkId).arg(best).arg(od.destinationExitLinkId);++m_reservations[key];}
    return best;
}

bool RoutingEngine::isForbiddenDestinationTarget(
    const VehicleOdState& od, qint64 targetLinkId) const
{
    if (!m_model || targetLinkId <= 0
        || targetLinkId == od.destinationExitLinkId)
        return false;
    return m_model->terminalIndexByExitLink().contains(targetLinkId)
        || m_model->forbiddenExitLinkIds().contains(targetLinkId);
}

void RoutingEngine::applyDecision(IVehicle* vehicle,
                                  IDecisionPoint* decisionPoint,
                                  IRouting* nativeRouting)
{
    QMutexLocker locker(&m_mutex);
    if (!vehicle || !decisionPoint || !m_model) return;
    registerVehicle(vehicle);
    if (!m_vehicleOd.contains(vehicle->id())) return;
    const qint64 decisionLinkId = decisionPoint->link()->id();
    const LocalDecision decision = m_model->decisionsByLink().value(decisionLinkId);
    if (decision.routes.isEmpty()) return;
    QVector<qint64> available;
    for (const LocalRouteOption& option : decision.routes) available.push_back(option.nextLinkId);
    const qint64 selectedNext = chooseNext(
        vehicle->id(), decision.routingContextLinkId, available);
    VehicleOdState& od = m_vehicleOd[vehicle->id()];
    const double retrySec = qMax(0.5,
        m_config.value("routeSyncRetrySec").toDouble(5.0));
    if (od.lastSyncFailureLinkId == decisionLinkId
        && od.lastSyncFailureTargetLinkId == selectedNext
        && m_nowSec - od.lastSyncFailureAtSec < retrySec) {
        // TESSNG may call afterDistriRouting repeatedly for the same stopped
        // vehicle.  A failed full-route assignment is retried on a bounded
        // cadence; hammering it every callback only inflates the fallback
        // counter and competes with the physical connector callback.
        return;
    }
    IRouting* selectedRouting = nullptr;
    for (IRouting* routing : decisionPoint->routings()) {
        if (!routing) continue;
        const QList<ILink*> links = routing->getLinks();
        if (!links.isEmpty() && links.last()->id() == selectedNext) {
            selectedRouting = routing;
            break;
        }
    }
    if (!selectedRouting) {
        ++m_routeFallbacks;
        od.lastSyncFailureLinkId = decisionLinkId;
        od.lastSyncFailureTargetLinkId = selectedNext;
        od.lastSyncFailureAtSec = m_nowSec;
        return;
    }
    QVector<qint64> prefix;
    for (ILink* link : selectedRouting->getLinks())
        if (link) prefix.push_back(link->id());
    QVector<qint64> executablePath;
    // A two-link decision routing ends at selectedNext.  If that next link
    // has no decision point, TESSNG removes the vehicle at its end.  Always
    // extend the local choice through the cached fixed-OD shortest-path tree
    // before assigning it to the vehicle.
    const bool applied = applyExecutableRouting(
        vehicle, decisionLinkId, prefix, &executablePath);
    if (applied) {
        if (nativeRouting && selectedRouting->id() != nativeRouting->id()) {
            ++od.routeChanges;
            ++m_routeChanges;
        }
        od.lockedDecisionLinkId = decisionLinkId;
        od.lockedNextLinkId = selectedNext;
        od.lastDecisionContextLinkId = decision.routingContextLinkId;
        od.lastDecisionAtSec = m_nowSec;
        od.lockedRouteLinkIds = executablePath;
        od.lastSyncFailureLinkId = 0;
        od.lastSyncFailureTargetLinkId = 0;
        od.lastSyncFailureAtSec = -1e9;
        ++m_routeDecisions;
    } else {
        ++m_routeFallbacks;
        od.lastSyncFailureLinkId = decisionLinkId;
        od.lastSyncFailureTargetLinkId = selectedNext;
        od.lastSyncFailureAtSec = m_nowSec;
    }
}

QList<ILaneConnector*> RoutingEngine::filterLaneConnectors(
    IVehicle* vehicle, const QList<ILaneConnector*>& candidates)
{
    QMutexLocker locker(&m_mutex);
    if (!vehicle || candidates.isEmpty() || !m_model) return candidates;
    registerVehicle(vehicle);
    if (!m_vehicleOd.contains(vehicle->id())) return candidates;
    const VehicleOdState odSnapshot = m_vehicleOd.value(vehicle->id());
    QVector<qint64> available;
    QHash<qint64, QList<ILaneConnector*>> byTarget;
    bool removedWrongExit = false;
    for (ILaneConnector* connector : candidates) {
        if (!connector || !connector->toLane() || !connector->toLane()->link()) continue;
        const qint64 target = connector->toLane()->link()->id();
        if (isForbiddenDestinationTarget(odSnapshot, target)) {
            removedWrongExit = true;
            continue;
        }
        if (!byTarget.contains(target)) available.push_back(target);
        byTarget[target].push_back(connector);
    }
    if (available.isEmpty()) {
        if (removedWrongExit) {
            // This is the final physical boundary. Returning an empty list
            // leaves the first vehicle permanently on the stop line even when
            // its signal is green, which then blocks the whole approach. The
            // upstream controller is responsible for avoiding this state; if
            // it is already unavoidable, fail open through a real connector.
            // The fixed OD is intentionally left unchanged, so MetricsRecorder
            // still classifies the eventual departure as a wrong destination
            // instead of a successful completion.
            vehicle->setJsonProperty(
                QStringLiteral("routingExecution"),
                QStringLiteral("WRONG_EXIT_PHYSICAL_FAIL_OPEN"));
            vehicle->setJsonProperty(
                QStringLiteral("wrongExitPhysicalFailOpen"), true);
            return candidates;
        }
        return candidates;
    }
    qint64 currentLink = vehicle->roadIsLink() ? vehicle->roadId() : 0;
    if (currentLink <= 0)
        for (ILaneConnector* connector : candidates)
            if (connector && connector->fromLane()
                && connector->fromLane()->link()) {
                currentLink = connector->fromLane()->link()->id();
                break;
            }
    VehicleOdState& od = m_vehicleOd[vehicle->id()];
    auto safePhysicalFallback = [&]() {
        // candidateLaneConnectors is the final executable boundary.  Once all
        // immediate wrong exits have been removed, it must never return an
        // empty list merely because the dynamic route tree is temporarily
        // incomplete.  Pick the physically available hop with the smallest
        // remaining fixed-OD cost; if every cached cost is unavailable, use a
        // stable link-id tie break so a green vehicle can still clear the
        // conflict area and be replanned at the next decision point.
        qint64 safestTarget = 0;
        double safestScore = std::numeric_limits<double>::infinity();
        for (qint64 target : available) {
            double remaining = target == od.destinationExitLinkId
                ? 0.0
                : m_distanceByDestination.value(od.destinationExitLinkId)
                    .value(target, std::numeric_limits<double>::infinity());
            if (!std::isfinite(remaining))
                remaining = m_freeFlowDistanceByDestination
                    .value(od.destinationExitLinkId)
                    .value(target, std::numeric_limits<double>::infinity());
            const double firstStep = m_edgeCost.value(currentLink)
                .value(target, 0.0);
            const double score = std::isfinite(remaining)
                ? firstStep + remaining
                : std::numeric_limits<double>::infinity();
            if (score < safestScore
                || (score == safestScore
                    && (safestTarget <= 0 || target < safestTarget))) {
                safestScore = score;
                safestTarget = target;
            }
        }
        if (safestTarget <= 0) {
            QVector<qint64> stableTargets = available;
            std::sort(stableTargets.begin(), stableTargets.end());
            safestTarget = stableTargets.value(0, 0);
        }
        if (safestTarget > 0 && byTarget.contains(safestTarget)) {
            vehicle->setJsonProperty(
                QStringLiteral("nextLinkId"), safestTarget);
            vehicle->setJsonProperty(
                QStringLiteral("routingExecution"),
                QStringLiteral("PHYSICAL_OD_SAFE_CONNECTOR_FALLBACK"));
            return byTarget.value(safestTarget);
        }
        // Defensive only: available and byTarget are populated together.
        return candidates;
    };
    auto selectExecutable = [&](qint64 target) {
        if (!byTarget.contains(target)) return safePhysicalFallback();
        if (currentLink <= 0) {
            // The physical callback already proves that this connector is
            // executable.  Missing road context only prevents a full-route
            // synchronization; it must not turn a green into a hard stop.
            return byTarget.value(target);
        }
        bool completeOdRoute = false;
        if (IRouting* routing = vehicle->routing()) {
            const QList<ILink*> links = routing->getLinks();
            completeOdRoute = !links.isEmpty() && links.last()
                && links.last()->id() == od.destinationExitLinkId;
        }
        const qint64 nativeTarget = nativeMovementTarget(vehicle, currentLink);
        if (nativeTarget != target || !completeOdRoute) {
            const double retrySec = qMax(0.5,
                m_config.value("routeSyncRetrySec").toDouble(5.0));
            const bool repeatedFailure = od.lastSyncFailureLinkId == currentLink
                && od.lastSyncFailureTargetLinkId == target
                && m_nowSec - od.lastSyncFailureAtSec < retrySec;
            if (repeatedFailure)
                return byTarget.value(target);
            if (!synchronizeNativeRoute(vehicle, currentLink, target)) {
                ++m_routeFallbacks;
                od.lastSyncFailureLinkId = currentLink;
                od.lastSyncFailureTargetLinkId = target;
                od.lastSyncFailureAtSec = m_nowSec;
                // A temporary full-route synchronization failure must not
                // remove a physically valid OD-safe connector from TESSNG.
                // Returning an empty list made vehicles stop on a real green
                // and amplified one transient routing failure into a queue.
                vehicle->setJsonProperty(
                    QStringLiteral("routingExecution"),
                    QStringLiteral("PHYSICAL_OD_SAFE_CONNECTOR_FALLBACK"));
                return byTarget.value(target);
            }
            od.lastSyncFailureLinkId = 0;
            od.lastSyncFailureTargetLinkId = 0;
            od.lastSyncFailureAtSec = -1e9;
        }
        return byTarget.value(target);
    };
    const qint64 owner = m_model->decisionOwnerByCorridorLink().value(currentLink, 0);
    if (owner > 0 && od.lockedDecisionLinkId == owner
        && m_config.value("lockDecisionUntilIncomingLinkExit").toBool(true)) {
        const LocalDecision lockedDecision = m_model->decisionsByLink().value(owner);
        // Follow the complete route through a relocated/widened decision
        // corridor. Returning every connector here allowed TESSNG to leave
        // the fixed-OD route before the final approach.
        for (int i = 0; i + 1 < od.lockedRouteLinkIds.size(); ++i)
            if (od.lockedRouteLinkIds[i] == currentLink
                && byTarget.contains(od.lockedRouteLinkIds[i + 1]))
                return selectExecutable(od.lockedRouteLinkIds[i + 1]);
        if (byTarget.contains(od.lockedNextLinkId))
            return selectExecutable(od.lockedNextLinkId);
        const qint64 replacement = chooseNext(vehicle->id(),
            currentLink == lockedDecision.routingContextLinkId
                ? lockedDecision.routingContextLinkId : currentLink,
            available);
        if (byTarget.contains(replacement)) return selectExecutable(replacement);
        ++m_routeFallbacks;
        return safePhysicalFallback();
    }
    qint64 selected = movementTargetLink(vehicle->id(), currentLink);
    if (!available.contains(selected)) selected = chooseNext(vehicle->id(), currentLink, available);
    if (byTarget.contains(selected)) return selectExecutable(selected);
    ++m_routeFallbacks;
    return safePhysicalFallback();
}
