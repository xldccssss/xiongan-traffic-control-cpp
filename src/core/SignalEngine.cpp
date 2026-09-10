#include "SignalEngine.h"

#include <QtMath>
#include <algorithm>
#include <cmath>
#include <limits>
#include <QMutexLocker>

namespace {
double clampValue(double v, double lo, double hi) { return qMax(lo, qMin(hi, v)); }
double cfg(const QJsonObject& o, const char* k, double d)
{ return o.value(QLatin1String(k)).toDouble(d); }
bool enabled(const QJsonObject& o, const char* k, bool d)
{ return o.value(QLatin1String(k)).toBool(d); }
double utility(const QVector<double>& scores)
{
    double first = 0.0, second = 0.0;
    for (double v : scores) {
        v = qMax(0.0, v);
        if (v > first) { second = first; first = v; }
        else if (v > second) second = v;
    }
    return first <= 1e-9 ? 0.0 : clampValue((first - second) / first, 0.0, 1.0);
}
}

SignalEngine::SignalEngine(const ProjectModel* model) { setModel(model); }

void SignalEngine::setBranch(ControlBranch branch)
{
    QMutexLocker locker(&m_mutex);
    m_branch = branch;
}

ControlBranch SignalEngine::branch() const
{
    QMutexLocker locker(&m_mutex);
    return m_branch;
}

void SignalEngine::setModel(const ProjectModel* model)
{
    QMutexLocker locker(&m_mutex);
    m_model = model;
    if (m_model) {
        m_signalConfig = m_model->scenario().value("signals").toObject();
        const QJsonObject runtime = m_model->scenario().value("runtimeControl").toObject();
        m_actuatedConfig = runtime.value("actuatedMaxPressure").toObject();
        m_pressureConfig = runtime.value("improvedMaxPressure").toObject();
        m_greenWaveConfig = m_pressureConfig.value(
            "entryArterialGreenWave").toObject();
        m_authorityConfig = runtime.value("dynamicAuthority").toObject();
        m_terminalConfig = m_model->scenario().value("terminalControl").toObject();
        m_permissiveRightTurn = enabled(m_signalConfig, "permissiveRightTurn", true);
        m_minGreenSec = qMax(1.0, cfg(m_signalConfig, "minGreenSec", 12.0));
        m_maxGreenSec = qMax(m_minGreenSec, cfg(m_signalConfig, "maxGreenSec", 45.0));
        m_decisionIntervalSec = qMax(0.5, cfg(m_actuatedConfig, "decisionIntervalSec", 5.0));
        m_switchRatio = qMax(1.0, cfg(m_actuatedConfig, "pressureSwitchRatio", 1.15));
        m_minimumGain = qMax(0.0, cfg(m_actuatedConfig, "minimumSwitchPressureGain", 0.5));
        m_branch = runtime.value("activeBranch").toString("IMPROVED_MAX_PRESSURE")
            == "FIXED_TIMING" ? ControlBranch::FixedTiming
                               : ControlBranch::ImprovedMaxPressure;
    }
    rebuildEntryGreenWaveTopology();
    reset();
}

void SignalEngine::rebuildEntryGreenWaveTopology()
{
    m_greenWaveSources.clear();
    m_entryGreenWaveGateways.clear();
    m_entryGreenWaveCorridorStages.clear();
    m_entryGreenWaveGatewayStageCount = 0;
    m_entryGreenWaveCorridorStageCount = 0;
    m_entryGreenWaveArcCount = 0;
    if (!m_model) return;

    const int intersectionCount = m_model->intersections().size();
    m_greenWaveSources.resize(intersectionCount);
    m_entryGreenWaveGateways.resize(intersectionCount);
    m_entryGreenWaveCorridorStages.resize(intersectionCount);
    for (int i = 0; i < intersectionCount; ++i) {
        const int stages = m_model->intersections()[i].stages.size();
        m_greenWaveSources[i].resize(stages);
        m_entryGreenWaveGateways[i].fill(false, stages);
        m_entryGreenWaveCorridorStages[i].fill(false, stages);
    }
    if (!enabled(m_greenWaveConfig, "enabled", true)) return;

    QHash<qint64, QVector<GraphEdge>> forwardEdges;
    QHash<qint64, QHash<qint64, double>> transitionTravelSec;
    for (const GraphEdge& edge : m_model->edges()) {
        forwardEdges[edge.from].push_back(edge);
        const GraphLink next = m_model->links().value(edge.to);
        const double speed = qMax(1.0, next.freeSpeedMps);
        const double travel = (qMax(0.0, edge.connectorLengthM)
                               + qMax(0.1, next.lengthM)) / speed;
        transitionTravelSec[edge.from].insert(edge.to, travel);
    }

    struct SearchNode {
        qint64 linkId = 0;
        int depth = 0;
        double travelTimeSec = 0.0;
    };
    struct StageHit {
        StageRef ref;
        qint64 approachLinkId = 0;
        double travelTimeSec = 0.0;
    };
    auto nearestSignalStages = [&](qint64 startLinkId, int maximumDepth,
                                   double initialTravelSec) {
        QVector<StageHit> hits;
        QVector<SearchNode> frontier;
        frontier.push_back({startLinkId, 0, initialTravelSec});
        QSet<qint64> visited;
        visited.insert(startLinkId);
        for (int depth = 0; depth <= maximumDepth && !frontier.isEmpty(); ++depth) {
            QVector<SearchNode> next;
            QSet<QString> hitKeys;
            for (const SearchNode& node : frontier) {
                for (const StageRef& ref
                     : m_model->incomingStagesByLink().value(node.linkId)) {
                    if (ref.intersection < 0 || ref.intersection >= intersectionCount
                        || !m_model->intersections()[ref.intersection].signalized
                        || m_model->intersections()[ref.intersection].permanentGreen)
                        continue;
                    const QString key = QStringLiteral("%1/%2/%3")
                        .arg(ref.intersection).arg(ref.stage).arg(node.linkId);
                    if (hitKeys.contains(key)) continue;
                    hitKeys.insert(key);
                    hits.push_back({ref, node.linkId, node.travelTimeSec});
                }
            }
            if (!hits.isEmpty()) break;
            for (const SearchNode& node : frontier) {
                for (const GraphEdge& edge : forwardEdges.value(node.linkId)) {
                    if (visited.contains(edge.to)) continue;
                    visited.insert(edge.to);
                    next.push_back({edge.to, depth + 1,
                        node.travelTimeSec
                            + transitionTravelSec.value(edge.from).value(edge.to)});
                }
            }
            frontier = next;
        }
        return hits;
    };

    struct CorridorNode {
        StageRef ref;
        qint64 approachLinkId = 0;
        int signalDepth = 0;
    };
    QVector<CorridorNode> work;
    const int gatewayDepth = qBound(0, m_greenWaveConfig.value(
        "gatewaySearchDepth").toInt(3), 8);
    const bool outerOnly = enabled(m_greenWaveConfig,
                                   "outerBoundaryOnly", true);
    for (const TerminalDefinition& terminal : m_model->terminals()) {
        if (outerOnly && terminal.group != QStringLiteral("OUTER_BOUNDARY"))
            continue;
        for (const StageHit& hit
             : nearestSignalStages(terminal.entryLinkId, gatewayDepth, 0.0)) {
            if (!m_entryGreenWaveGateways[hit.ref.intersection]
                    [hit.ref.stage]) {
                m_entryGreenWaveGateways[hit.ref.intersection][hit.ref.stage] = true;
                ++m_entryGreenWaveGatewayStageCount;
            }
            m_entryGreenWaveCorridorStages[hit.ref.intersection][hit.ref.stage] = true;
            work.push_back({hit.ref, hit.approachLinkId, 0});
        }
    }

    const int maximumSignalDepth = qBound(1, m_greenWaveConfig.value(
        "maximumSignalDepth").toInt(10), 32);
    const int unsignalizedDepth = qBound(0, m_greenWaveConfig.value(
        "unsignalizedSearchDepth").toInt(2), 8);
    QSet<QString> visitedNodes;
    QSet<QString> arcKeys;
    for (int cursor = 0; cursor < work.size(); ++cursor) {
        const CorridorNode node = work[cursor];
        const QString nodeKey = QStringLiteral("%1/%2/%3")
            .arg(node.ref.intersection).arg(node.ref.stage)
            .arg(node.approachLinkId);
        if (visitedNodes.contains(nodeKey)) continue;
        visitedNodes.insert(nodeKey);
        if (node.signalDepth >= maximumSignalDepth) continue;
        const StageDefinition stage = m_model->intersections()
            [node.ref.intersection].stages.value(node.ref.stage);
        for (const QPair<qint64, qint64>& movement
             : stage.straightMovementLinkPairs) {
            if (movement.first != node.approachLinkId) continue;
            const GraphLink nextLink = m_model->links().value(movement.second);
            const double fallbackTravel = qMax(0.1, nextLink.lengthM)
                / qMax(1.0, nextLink.freeSpeedMps);
            const double initialTravel = transitionTravelSec
                .value(movement.first).value(movement.second, fallbackTravel);
            for (const StageHit& hit : nearestSignalStages(
                     movement.second, unsignalizedDepth, initialTravel)) {
                if (hit.ref.intersection == node.ref.intersection
                    && hit.ref.stage == node.ref.stage)
                    continue;
                const QString arcKey = QStringLiteral("%1/%2>%3/%4")
                    .arg(node.ref.intersection).arg(node.ref.stage)
                    .arg(hit.ref.intersection).arg(hit.ref.stage);
                if (!arcKeys.contains(arcKey)) {
                    arcKeys.insert(arcKey);
                    m_greenWaveSources[hit.ref.intersection][hit.ref.stage]
                        .push_back({node.ref.intersection, node.ref.stage,
                                    qMax(0.1, hit.travelTimeSec)});
                    ++m_entryGreenWaveArcCount;
                }
                m_entryGreenWaveCorridorStages[hit.ref.intersection]
                    [hit.ref.stage] = true;
                work.push_back({hit.ref, hit.approachLinkId,
                                node.signalDepth + 1});
            }
        }
    }
    for (const QVector<bool>& stages : m_entryGreenWaveCorridorStages)
        for (bool on : stages)
            if (on) ++m_entryGreenWaveCorridorStageCount;
}

void SignalEngine::reset(double nowSec)
{
    QMutexLocker locker(&m_mutex);
    m_states.clear();
    m_authorityLeaseSerial = 0;
    if (!m_model) return;
    m_states.resize(m_model->intersections().size());
    for (int i = 0; i < m_states.size(); ++i) {
        IntersectionSignalState& st = m_states[i];
        const IntersectionDefinition& def = m_model->intersections()[i];
        const int n = def.stages.size();
        st.activeStage = 0;
        st.pendingStage = -1;
        st.transition = SignalTransition::Green;
        st.stateStartedSec = st.greenStartedSec = nowSec;
        st.lastDecisionSec = nowSec - m_decisionIntervalSec;
        st.previousQueue.fill(0.0, n);
        st.previousDownstreamOccupancy.fill(0.0, n);
        st.previousScore.fill(0.05, n);
        st.underservedCycles.fill(0.0, n);
        st.plannedGreenSec.resize(n);
        st.lastQueues.fill(0.0, n);
        st.lastScores.fill(0.0, n);
        st.lastEffectiveScores.fill(0.0, n);
        st.stageRedStartedSec.fill(nowSec, n);
        if (n) st.stageRedStartedSec[0] = -1.0;
        st.stageLastGreenStartedSec.fill(-1.0, n);
        if (n) st.stageLastGreenStartedSec[0] = nowSec;
        // Python keeps this cache empty until the first coordination sample;
        // -1 marks the same "not observed yet" state without changing vector size.
        st.terminalMultipliers.fill(-1.0, n);
        st.stageSkipCount.fill(0, n);
        st.stageServiceCount.fill(0, n);
        if (n) st.stageServiceCount[0] = 1;
        st.pressureInitialized.fill(false, n);
        st.redAgeSec.fill(0.0, n);
        st.spillbackBlocked.fill(false, n);
        st.plannedCycleSec = def.cycleSec;
        st.lastPlanUpdateSec = nowSec - qMax(1, def.cycleSec);
        for (int s = 0; s < n; ++s) st.plannedGreenSec[s] = def.stages[s].greenSec;
        st.emptyStageStartedSec = -1.0;
        st.edgeIsLeader = false;
        st.authorityInitialized = false;
        st.authorityLeaseId = 0;
        st.authorityLeaseValidFromSec = st.authorityLeaseUntilSec = 0.0;
        st.authorityLastEvaluationSec = -1e9;
        st.authorityChallenger = -1;
        st.authorityAdvantageCount = 0;
        st.authoritySwitchCount = 0;
    }
}

void SignalEngine::advanceTransition(int index, double now)
{
    IntersectionSignalState& st = m_states[index];
    const IntersectionDefinition& def = m_model->intersections()[index];
    const double queueThreshold = qMax(0.0,
        cfg(m_actuatedConfig, "starvationQueueThresholdVeh", 1.0));
    while (true) {
        const double elapsed = now - st.stateStartedSec;
        if (st.transition == SignalTransition::Yellow
            && elapsed + 1e-9 >= def.yellowSec) {
            st.transition = SignalTransition::AllRed;
            st.stateStartedSec += def.yellowSec;
            continue;
        }
        if (st.transition == SignalTransition::AllRed
            && elapsed + 1e-9 >= def.allRedSec) {
            st.activeStage = st.pendingStage >= 0 ? st.pendingStage
                : (st.activeStage + 1) % qMax(1, def.stages.size());
            st.pendingStage = -1;
            st.transition = SignalTransition::Green;
            st.stateStartedSec += def.allRedSec;
            st.greenStartedSec = st.stateStartedSec;
            st.stageLastGreenStartedSec[st.activeStage] = st.greenStartedSec;
            st.emptyStageStartedSec = -1.0;
            st.stageRedStartedSec[st.activeStage] = -1.0;
            st.stageSkipCount[st.activeStage] = 0;
            ++st.stageServiceCount[st.activeStage];
            for (int s = 0; s < st.stageSkipCount.size(); ++s) {
                if (s == st.activeStage) continue;
                st.stageSkipCount[s] = st.lastQueues.value(s) >= queueThreshold
                    ? st.stageSkipCount.value(s) + 1 : 0;
            }
            continue;
        }
        break;
    }
}

void SignalEngine::beginSwitch(int index, int next, double now)
{
    IntersectionSignalState& st = m_states[index];
    const int n = m_model->intersections()[index].stages.size();
    if (st.transition != SignalTransition::Green || n <= 1
        || next < 0 || next >= n || next == st.activeStage) return;
    st.pendingStage = next;
    st.transition = SignalTransition::Yellow;
    st.stateStartedSec = now;
    if (st.stageRedStartedSec.value(st.activeStage) < 0.0)
        st.stageRedStartedSec[st.activeStage] = now;
}

void SignalEngine::updateFixed(int index, double now)
{
    IntersectionSignalState& st = m_states[index];
    const IntersectionDefinition& def = m_model->intersections()[index];
    if (st.transition != SignalTransition::Green || def.stages.size() <= 1) return;
    if (now - st.greenStartedSec + 1e-9 >= qMax(1, def.stages[st.activeStage].greenSec))
        beginSwitch(index, (st.activeStage + 1) % def.stages.size(), now);
}

QVector<double> SignalEngine::congestionByIntersection(
    const QVector<QVector<StageTraffic>>& values) const
{
    const int intersectionCount = m_model ? m_model->intersections().size() : 0;
    QVector<double> out(intersectionCount, 0.0);
    for (int i = 0; i < intersectionCount; ++i) {
        double queue = 0.0, maxOccupancy = 0.0;
        int lanes = 0;
        const QVector<StageTraffic> stageValues = values.value(i);
        const int stageCount = m_model->intersections().value(i).stages.size();
        for (int s = 0; s < stageCount; ++s) {
            const StageTraffic stage = stageValues.value(s);
            queue += qMax(0.0, stage.queueVeh);
            lanes += qMax(1, m_model->intersections()[i]
                .stages.value(s).incomingLaneIds.size());
            maxOccupancy = qMax(maxOccupancy,
                                clampValue(stage.downstreamOccupancy, 0.0, 1.0));
        }
        out[i] = clampValue(0.45 * clampValue(queue / (qMax(1, lanes) * 18.0), 0.0, 1.0)
                            + 0.55 * maxOccupancy, 0.0, 1.0);
    }
    return out;
}

bool SignalEngine::isEntryGreenWaveGateway(int index, int stage) const
{
    return index >= 0 && index < m_entryGreenWaveGateways.size()
        && stage >= 0 && stage < m_entryGreenWaveGateways[index].size()
        && m_entryGreenWaveGateways[index][stage];
}

bool SignalEngine::greenWaveArrivalActive(int index, int stage,
                                          double nowSec) const
{
    if (!enabled(m_greenWaveConfig, "enabled", true)
        || index < 0 || index >= m_greenWaveSources.size()
        || stage < 0 || stage >= m_greenWaveSources[index].size())
        return false;
    const double lead = qMax(0.0, cfg(m_greenWaveConfig,
                                      "arrivalLeadSec", 3.0));
    const double tail = qMax(0.0, cfg(m_greenWaveConfig,
                                      "tailClearanceSec", 5.0));
    for (const GreenWaveArc& arc : m_greenWaveSources[index][stage]) {
        if (arc.upstreamIntersection < 0
            || arc.upstreamIntersection >= m_states.size())
            continue;
        const IntersectionSignalState& source =
            m_states[arc.upstreamIntersection];
        if (arc.upstreamStage < 0
            || arc.upstreamStage >= source.stageLastGreenStartedSec.size()
            || source.stageServiceCount.value(arc.upstreamStage) <= 0)
            continue;
        const double releaseStart = source.stageLastGreenStartedSec
            .value(arc.upstreamStage, -1.0);
        if (releaseStart < 0.0) continue;
        const bool stillGreen = source.transition == SignalTransition::Green
            && source.activeStage == arc.upstreamStage;
        const double releaseEnd = stillGreen ? nowSec
            : source.stageRedStartedSec.value(arc.upstreamStage, -1.0);
        if (releaseEnd + 1e-9 < releaseStart) continue;
        const double arrivalStart = releaseStart + arc.travelTimeSec - lead;
        const double arrivalEnd = releaseEnd + arc.travelTimeSec + tail;
        if (nowSec + 1e-9 >= arrivalStart
            && nowSec <= arrivalEnd + 1e-9)
            return true;
    }
    return false;
}

QVector<StagePressureEvidence> SignalEngine::improvedEvidence(
    int index, const QVector<StageTraffic>& traffic, double nowSec)
{
    IntersectionSignalState& mem = m_states[index];
    const IntersectionDefinition& def = m_model->intersections()[index];
    QVector<StagePressureEvidence> out(traffic.size());
    const bool prediction = enabled(m_pressureConfig, "predictionEnabled", true);
    const bool spillback = enabled(m_pressureConfig, "spillbackGateEnabled", true);
    const bool fairnessOn = enabled(m_pressureConfig, "fairnessEnabled", true);
    const bool reliabilityOn = enabled(m_pressureConfig, "rsuReliabilityEnabled", true);
    const bool parallelOn = enabled(m_pressureConfig, "compatibleParallelismEnabled", true);
    const double horizon = cfg(m_pressureConfig, "predictionHorizonSec", 15.0);
    const double downstreamWeight = cfg(m_pressureConfig, "downstreamQueueWeight", 0.75);
    const double arrivalWeight = cfg(m_pressureConfig, "arrivalPredictionWeight", 1.0);
    const double growthWeight = cfg(m_pressureConfig, "queueGrowthWeight", 0.8);
    const double freeFlow = qMax(0.1, cfg(m_pressureConfig, "freeFlowSpeedMps", 13.9));
    const double minWeight = qMax(0.001, cfg(m_pressureConfig, "minimumWeight", 0.05));
    const double smoothing = clampValue(cfg(m_pressureConfig, "previousWeightSmoothing", 0.35), 0.0, 0.9);
    const double block = clampValue(cfg(m_pressureConfig, "spillbackBlockOccupancy", 0.78), 0.05, 1.0);
    const double release = clampValue(cfg(m_pressureConfig, "spillbackReleaseOccupancy", 0.68), 0.0, block);
    const double controlInterval = qMax(0.1, cfg(m_signalConfig, "controlIntervalSec", 10.0));
    for (int s = 0; s < traffic.size(); ++s) {
        const StageTraffic& v = traffic[s];
        StagePressureEvidence& e = out[s];
        e.queueVeh = qMax(0.0, v.queueVeh);
        const double oldQueue = mem.pressureInitialized.value(s)
            ? mem.previousQueue.value(s) : e.queueVeh;
        e.queueTrendVeh = prediction ? e.queueVeh - oldQueue : 0.0;
        e.predictedArrivalsVeh = prediction
            ? qMax(0.0, v.arrivalsVehPerMin) * horizon / 60.0 : 0.0;
        const double oldOcc = mem.pressureInitialized.value(s)
            ? mem.previousDownstreamOccupancy.value(s) : v.downstreamOccupancy;
        const double predictedIncrease = prediction ? qMin(
            clampValue(cfg(m_pressureConfig, "maximumPredictedOccupancyIncrease", 0.15), 0.0, 1.0),
            qMax(0.0, cfg(m_pressureConfig, "occupancyPredictionGain", 0.35))
                * qMax(0.0, v.downstreamOccupancy - oldOcc)
                * qMax(0.0, cfg(m_pressureConfig, "spillbackPredictionHorizonSec", 10.0))
                / controlInterval) : 0.0;
        e.predictedDownstreamOccupancy = clampValue(v.downstreamOccupancy + predictedIncrease, 0.0, 1.0);
        const double pressure = qMax(0.0, e.queueVeh
            + arrivalWeight * e.predictedArrivalsVeh
            + growthWeight * qMax(0.0, e.queueTrendVeh)
            - downstreamWeight * qMax(0.0, v.downstreamQueueVeh));
        const double speedDeficit = v.vehicleCount > 0.0
            ? 1.0 - clampValue(v.meanSpeedMps / freeFlow, 0.0, 1.0) : 0.0;
        const double minGate = clampValue(cfg(m_pressureConfig, "minimumDischargeGate", 0.05), 0.0, 1.0);
        double gate = clampValue(1.0 - cfg(m_pressureConfig, "occupancyPenaltyWeight", 0.85)
            * qPow(v.downstreamOccupancy,
                   qMax(1.0, cfg(m_pressureConfig, "occupancyExponent", 2.0))), minGate, 1.0);
        if (spillback) {
            mem.spillbackBlocked[s] = mem.spillbackBlocked.value(s)
                ? e.predictedDownstreamOccupancy > release
                : e.predictedDownstreamOccupancy >= block;
        } else {
            mem.spillbackBlocked[s] = false;
            gate = 1.0;
        }
        e.spillbackBlocked = mem.spillbackBlocked[s];
        if (e.spillbackBlocked) gate = enabled(
            m_pressureConfig, "hardSpillbackGateEnabled", true) ? 0.0 : minGate;
        double age = mem.underservedCycles.value(s);
        if (e.queueVeh >= cfg(m_pressureConfig, "fairnessQueueThresholdVeh", 3.0)
            && mem.plannedGreenSec.value(s, m_minGreenSec)
                <= m_minGreenSec + cfg(m_pressureConfig, "fairnessMinGreenMarginSec", 2.0))
            age = qMin(qMax(1.0, cfg(m_pressureConfig, "fairnessMaxCycles", 3.0)),
                       age + controlInterval / qMax(1.0, mem.plannedCycleSec));
        else age = qMax(0.0, age - controlInterval / qMax(1.0, mem.plannedCycleSec));
        mem.underservedCycles[s] = age;
        const double fair = fairnessOn ? 1.0
            + qMax(0.0, cfg(m_pressureConfig, "fairnessGain", 0.20)) * age
                / qMax(1.0, cfg(m_pressureConfig, "fairnessMaxCycles", 3.0)) : 1.0;
        const double parallel = parallelOn ? 1.0
            + qMax(0.0, cfg(m_pressureConfig, "compatibleParallelismGain", 0.12))
                * (qMax(1, def.stages[s].concurrentApproaches) - 1) : 1.0;
        e.measurementReliability = reliabilityOn
            ? clampValue(v.measurementReliability > 0.0 ? v.measurementReliability
                : cfg(m_pressureConfig, "virtualRsuReliability", 0.85), 0.1, 1.0) : 1.0;
        const double greenWaveDemand = e.queueVeh + e.predictedArrivalsVeh;
        const double minimumWaveDemand = qMax(0.0, cfg(
            m_greenWaveConfig, "minimumDemandVeh", 1.0));
        e.greenWaveArrivalActive = greenWaveDemand >= minimumWaveDemand
            && greenWaveArrivalActive(index, s, nowSec);
        double greenWaveMultiplier = 1.0;
        if (enabled(m_greenWaveConfig, "enabled", true)
            && greenWaveDemand >= minimumWaveDemand) {
            if (isEntryGreenWaveGateway(index, s))
                greenWaveMultiplier += qMax(0.0, cfg(
                    m_greenWaveConfig, "entryGatewayPressureGain", 0.25));
            if (e.greenWaveArrivalActive)
                greenWaveMultiplier += qMax(0.0, cfg(
                    m_greenWaveConfig, "progressionPressureGain", 0.35));
        }
        e.greenWaveMultiplier = clampValue(greenWaveMultiplier, 1.0,
            qMax(1.0, cfg(m_greenWaveConfig,
                          "maximumPressureMultiplier", 1.55)));
        e.rawWeight = (minWeight + pressure
            * (1.0 + cfg(m_pressureConfig, "speedDeficitWeight", 0.35) * speedDeficit)
            * gate * fair * parallel * e.measurementReliability)
            * e.greenWaveMultiplier;
        e.weight = mem.pressureInitialized.value(s)
            ? smoothing * mem.previousScore.value(s, minWeight)
                + (1.0 - smoothing) * e.rawWeight : e.rawWeight;
        e.weight = qMax(minWeight, e.weight);
        e.terminalWeight = e.weight * mem.terminalMultipliers.value(s, 1.0);
        mem.previousQueue[s] = e.queueVeh;
        mem.previousDownstreamOccupancy[s] = v.downstreamOccupancy;
        mem.previousScore[s] = e.weight;
        mem.pressureInitialized[s] = true;
    }
    return out;
}

void SignalEngine::updatePlanMemory(int index, const QVector<StageTraffic>& traffic,
    const QVector<StagePressureEvidence>& evidence, double networkCongestion,
    double nowSec)
{
    IntersectionSignalState& st = m_states[index];
    const IntersectionDefinition& def = m_model->intersections()[index];
    const int n = def.stages.size();
    if (!n) return;
    // maxGreenChangeSecPerCycle is a cycle-to-cycle constraint.  Recomputing
    // this plan every 5-second pressure decision used to ramp a 15-second
    // green to the global 45-second ceiling within one cycle.
    if (nowSec + 1e-9 < st.lastPlanUpdateSec
            + qMax(1.0, st.plannedCycleSec))
        return;
    double desired = st.plannedCycleSec;
    if (enabled(m_pressureConfig, "dynamicCycleEnabled", true)) {
        double demand = 0.0; int lanes = 0;
        const double horizon = cfg(m_pressureConfig, "predictionHorizonSec", 15.0);
        for (int s = 0; s < traffic.size(); ++s) {
            demand += qMax(0.0, traffic[s].queueVeh)
                + qMax(0.0, traffic[s].arrivalsVehPerMin) * horizon / 60.0;
            lanes += qMax(1, def.stages[s].incomingLaneIds.size());
        }
        desired = cfg(m_signalConfig, "targetCycleSec", 80.0)
            + cfg(m_pressureConfig, "cycleGainSecPerVehPerLane", 3.0)
                * (demand / qMax(1, lanes)
                   - cfg(m_pressureConfig, "neutralDemandVehPerLane", 4.0));
    }
    const double minCycle = cfg(m_pressureConfig, "minimumCycleSec", 60.0);
    const double maxCycle = cfg(m_pressureConfig, "maximumCycleSec", 110.0);
    const double maxChange = qMax(0.0, cfg(m_pressureConfig, "maxCycleChangeSecPerUpdate", 8.0));
    desired = clampValue(desired, minCycle, maxCycle);
    desired = clampValue(desired, st.plannedCycleSec - maxChange, st.plannedCycleSec + maxChange);
    const double terminalBias = clampValue(
        (networkCongestion - cfg(m_terminalConfig, "neutralNetworkCongestion", 0.30))
            * cfg(m_terminalConfig, "networkCycleBiasGainSec", 20.0),
        -qAbs(cfg(m_terminalConfig, "maximumNetworkCycleBiasSec", 8.0)),
         qAbs(cfg(m_terminalConfig, "maximumNetworkCycleBiasSec", 8.0)));
    desired = clampValue(desired + terminalBias, minCycle, maxCycle);
    desired = clampValue(desired, st.plannedCycleSec - maxChange, st.plannedCycleSec + maxChange);
    const int minRequired = n * (qRound(m_minGreenSec) + def.yellowSec + def.allRedSec);
    const int cycle = qRound(qMax<double>(minRequired, desired));
    const int effective = qMax(n * qRound(m_minGreenSec),
        cycle - n * (def.yellowSec + def.allRedSec));
    QVector<double> weights(n, 0.05); double total = 0.0;
    for (int s = 0; s < n; ++s) { weights[s] = qMax(0.001, evidence[s].terminalWeight); total += weights[s]; }
    const int greenChange = qMax(0, qRound(cfg(m_signalConfig, "maxGreenChangeSecPerCycle", 6.0)));
    QVector<int> values(n), lower(n), upper(n), order(n);
    for (int s = 0; s < n; ++s) {
        const int old = qRound(st.plannedGreenSec.value(s, m_minGreenSec));
        lower[s] = qMax(qRound(m_minGreenSec), old - greenChange);
        upper[s] = qMin(qRound(m_maxGreenSec), old + greenChange);
        values[s] = qBound(lower[s], qRound(effective * weights[s] / qMax(1e-9, total)), upper[s]);
        order[s] = s;
    }
    std::stable_sort(order.begin(), order.end(), [&weights](int a, int b){ return weights[a] > weights[b]; });
    int delta = effective; for (int v : values) delta -= v;
    for (int guard = 0; delta != 0 && guard < 10000; ++guard) {
        bool changed = false;
        if (delta > 0) for (int s : order) { if (values[s] < upper[s]) { ++values[s]; --delta; changed = true; if (!delta) break; } }
        else for (auto it = order.crbegin(); it != order.crend(); ++it) { const int s=*it; if (values[s] > lower[s]) { --values[s]; ++delta; changed = true; if (!delta) break; } }
        if (!changed) break;
    }
    for (int s = 0; s < n; ++s) st.plannedGreenSec[s] = values[s];
    st.plannedCycleSec = cycle;
    st.lastPlanUpdateSec = nowSec;
}

QVector<double> SignalEngine::coordinateAuthority(int index, double now,
    const QVector<StageTraffic>& traffic, QVector<StagePressureEvidence>* ev,
    double networkCongestion, const QVector<double>& congestion)
{
    QVector<double> edge(ev->size()), terminal(ev->size());
    for (int s = 0; s < ev->size(); ++s) {
        StagePressureEvidence& e = (*ev)[s];
        const double q = qMax(0.0, e.queueVeh), g = qMax(0.0, e.queueTrendVeh), a = qMax(0.0, e.predictedArrivalsVeh);
        e.edgeWeight = e.weight * (1.0
            + qMax(0.0, cfg(m_authorityConfig, "edgeQueueGain", 0.20)) * q/(q+10.0)
            + qMax(0.0, cfg(m_authorityConfig, "edgeGrowthGain", 0.15)) * g/(g+5.0)
            + qMax(0.0, cfg(m_authorityConfig, "edgeArrivalGain", 0.10)) * a/(a+5.0));
        edge[s] = e.edgeWeight; terminal[s] = e.terminalWeight;
    }
    if (!enabled(m_authorityConfig, "enabled", true)) return terminal;
    double reliability = 0.0, peakQueue = 0.0;
    for (int s=0; s<ev->size(); ++s) {
        reliability += (*ev)[s].measurementReliability;
        peakQueue = qMax(peakQueue, traffic[s].queueVeh);
        for (double v : traffic[s].queueByLane) peakQueue = qMax(peakQueue, v);
    }
    reliability /= qMax(1, ev->size());
    QHash<QString,int> byId;
    for (int i=0;i<m_model->intersections().size();++i) byId.insert(m_model->intersections()[i].id,i);
    double downstream=0.0;
    for (const StageDefinition& s : m_model->intersections()[index].stages)
        for (const QString& id : s.downstreamIntersectionIds)
            downstream=qMax(downstream,congestion.value(byId.value(id,-1)));
    const double emergencyQueue=qMax(1.0,cfg(m_authorityConfig,"emergencyLocalQueueVeh",18.0));
    const double edgeScore=clampValue(0.22*0.92+0.18*0.90+0.20*reliability
        +0.20*utility(edge)+0.20*clampValue(peakQueue/emergencyQueue,0.0,1.0),0.0,1.0);
    const double terminalScore=clampValue(0.22+0.18+0.22*clampValue(0.90+0.10*downstream,0.0,1.0)
        +0.20*utility(terminal)+0.18*qMax(networkCongestion,downstream),0.0,1.0);
    IntersectionSignalState& st=m_states[index];
    const double lease=qMax(1.0,cfg(m_authorityConfig,"leaseDurationSec",15.0));
    auto issue=[&](int leader){ st.edgeIsLeader=leader==1; st.authorityInitialized=true;
        st.authorityLeaseId=++m_authorityLeaseSerial; st.authorityLeaseValidFromSec=now;
        st.authorityLeaseUntilSec=now+lease; st.authorityLastEvaluationSec=now;
        st.authorityChallenger=-1; st.authorityAdvantageCount=0; };
    if (!st.authorityInitialized) issue(edgeScore>terminalScore?1:0);
    else if (now+1e-9>=st.authorityLastEvaluationSec+qMax(0.5,cfg(m_authorityConfig,"evaluationIntervalSec",5.0))) {
        st.authorityLastEvaluationSec=now;
        const int leader=st.edgeIsLeader?1:0, follower=1-leader;
        const bool emergency=(follower==1 && peakQueue>=emergencyQueue)
            || (follower==0 && qMax(networkCongestion,downstream)>=clampValue(cfg(m_authorityConfig,"emergencyRegionalCongestion",0.85),0.0,1.0));
        if (emergency) { ++st.authoritySwitchCount; issue(follower); }
        else {
            const double ls=leader?edgeScore:terminalScore, fs=follower?edgeScore:terminalScore;
            if (fs>=ls+qMax(0.0,cfg(m_authorityConfig,"minimumScoreAdvantage",0.15))) {
                if (st.authorityChallenger==follower) ++st.authorityAdvantageCount;
                else { st.authorityChallenger=follower; st.authorityAdvantageCount=1; }
            } else { st.authorityChallenger=-1; st.authorityAdvantageCount=0; }
            const int wins=qMax(1,m_authorityConfig.value("consecutiveAdvantageCycles").toInt(3));
            if (now+1e-9>=st.authorityLeaseUntilSec && st.authorityChallenger==follower && st.authorityAdvantageCount>=wins)
            { ++st.authoritySwitchCount; issue(follower); }
            else if (now+1e-9>=st.authorityLeaseUntilSec) {
                st.authorityLeaseId=++m_authorityLeaseSerial; st.authorityLeaseValidFromSec=now; st.authorityLeaseUntilSec=now+lease;
            }
        }
    }
    const double followerConfidence=st.edgeIsLeader?1.0:reliability;
    const double correction=clampValue(cfg(m_authorityConfig,"maximumFollowerCorrectionWeight",0.30),0.0,0.49)
        *clampValue(followerConfidence,0.0,1.0);
    QVector<double> fused(ev->size());
    for (int s=0;s<ev->size();++s) {
        const double lead=st.edgeIsLeader?edge[s]:terminal[s], follow=st.edgeIsLeader?terminal[s]:edge[s];
        fused[s]=qMax(0.0,lead+correction*(follow-lead)); (*ev)[s].fusedWeight=fused[s];
    }
    return fused;
}

QVector<double> SignalEngine::effectiveScores(int index,double now,const QVector<double>& scores,
    const QVector<StageTraffic>& traffic,double* soft,double* hard)
{
    IntersectionSignalState& st=m_states[index]; const int n=scores.size();
    const IntersectionDefinition& def=m_model->intersections()[index];
    const double clearance=def.yellowSec+def.allRedSec;
    const double quantized=qCeil((m_minGreenSec+clearance)/m_decisionIntervalSec)*m_decisionIntervalSec;
    const double configuredSoft=qMax(m_minGreenSec,cfg(m_actuatedConfig,"starvationSoftRedSec",35.0));
    *hard=qMax(qMax(configuredSoft,cfg(m_actuatedConfig,"starvationHardRedSec",100.0)),
               qMax(0,n-1)*quantized+clearance+m_decisionIntervalSec);
    *soft=qMin(configuredSoft,qMax(m_minGreenSec,*hard-m_minGreenSec));
    QVector<double> out(n); const bool on=enabled(m_actuatedConfig,"boundedStarvationEnabled",true);
    const double qMinStarve=qMax(0.0,cfg(m_actuatedConfig,"starvationQueueThresholdVeh",1.0));
    const int maxSkips=qMax(1,m_actuatedConfig.value("maximumConsecutiveStageSkips").toInt(3));
    for(int s=0;s<n;++s){ const double age=(s==st.activeStage&&st.transition==SignalTransition::Green)?0.0:
        qMax(0.0,now-qMax(0.0,st.stageRedStartedSec.value(s,now))); st.redAgeSec[s]=age;
        const double raw=qMax(0.0,scores[s]), queue=qMax(0.0,traffic[s].queueVeh);
        if(!on||queue<qMinStarve||s==st.activeStage){out[s]=raw;continue;}
        const double ageRatio=qMin(2.0,qMax(0.0,age-*soft)/qMax(1.0,*soft));
        const double skipRatio=qMin(1.0,st.stageSkipCount.value(s)/double(maxSkips));
        out[s]=raw*(1.0+qMax(0.0,cfg(m_actuatedConfig,"redAgePressureGain",0.35))*ageRatio
            +qMax(0.0,cfg(m_actuatedConfig,"stageSkipPressureGain",0.20))*skipRatio)
            +qMax(0.0,cfg(m_actuatedConfig,"queueAgeAdditiveGain",0.10))*queue*qMin(1.0,age/qMax(1.0,*hard)); }
    return out;
}

int SignalEngine::serviceDebtCandidate(int index,const QVector<int>& candidates,
    const QVector<double>& effective,const QVector<StageTraffic>& traffic) const
{
    if(candidates.isEmpty())return -1; const IntersectionSignalState& st=m_states[index];
    const double threshold=qMax(0.0,cfg(m_actuatedConfig,"starvationQueueThresholdVeh",1.0));
    QVector<int> pool; for(int s:candidates)if(traffic[s].queueVeh>=threshold)pool.push_back(s);
    if(pool.isEmpty())pool=candidates; int best=pool.first();
    auto better=[&](int a,int b){if(st.stageSkipCount[a]!=st.stageSkipCount[b])return st.stageSkipCount[a]>st.stageSkipCount[b];
        if(st.redAgeSec[a]!=st.redAgeSec[b])return st.redAgeSec[a]>st.redAgeSec[b];
        if(traffic[a].queueVeh!=traffic[b].queueVeh)return traffic[a].queueVeh>traffic[b].queueVeh;
        return effective[a]>effective[b];}; for(int s:pool)if(better(s,best))best=s; return best;
}

void SignalEngine::updateImproved(int index,double now,const QVector<StageTraffic>& rawTraffic,
    double networkCongestion,const QVector<double>& congestion)
{
    IntersectionSignalState& st=m_states[index]; const IntersectionDefinition& def=m_model->intersections()[index];
    if(st.transition!=SignalTransition::Green||def.stages.size()<=1||now-st.lastDecisionSec+1e-9<m_decisionIntervalSec)return;
    st.lastDecisionSec=now;
    // Runtime aggregation and signal construction are refreshed by separate
    // callbacks.  Keep one zero-filled traffic slot per configured stage so a
    // transient short sample can never index beyond a QVector during a dynamic
    // plan update (typically first exposed at the next 80 s cycle boundary).
    QVector<StageTraffic> traffic=rawTraffic;
    traffic.resize(def.stages.size());
    QVector<StagePressureEvidence> ev=improvedEvidence(index,traffic,now);
    updatePlanMemory(index,traffic,ev,networkCongestion,now);
    QVector<double> scores=coordinateAuthority(index,now,traffic,&ev,networkCongestion,congestion);
    for(StageTraffic& v:traffic){double service=0.0;for(double q:v.serviceableQueueByLane)service+=q;if(!v.serviceableQueueByLane.isEmpty())v.queueVeh=service;}
    st.lastScores=scores; st.lastQueues.resize(traffic.size()); for(int s=0;s<traffic.size();++s)st.lastQueues[s]=traffic[s].queueVeh;
    double soft=0.0,hard=0.0; QVector<double> effective=effectiveScores(index,now,scores,traffic,&soft,&hard); st.lastEffectiveScores=effective;
    const int current=st.activeStage; QVector<int> feasible; for(int s=0;s<ev.size();++s)if(!ev[s].spillbackBlocked)feasible.push_back(s);
    const bool allStagesBlocked=feasible.isEmpty();
    QVector<int> candidates=feasible; if(candidates.isEmpty())for(int s=0;s<ev.size();++s)candidates.push_back(s);
    int best=candidates.value(0,current);for(int s:candidates)if(effective[s]>effective[best])best=s;
    if(enabled(m_actuatedConfig,"bidirectionalStagePreferenceEnabled",true)){
        const double floor=clampValue(cfg(m_actuatedConfig,"bidirectionalPreferenceFloorRatio",0.85),0.0,1.0);int pb=-1;
        for(int s:candidates)if(def.stages[s].concurrentApproaches>1&&traffic[s].queueVeh>=cfg(m_actuatedConfig,"starvationQueueThresholdVeh",1.0)
            &&effective[s]+1e-9>=effective[best]*floor&&(pb<0||def.stages[s].concurrentApproaches>def.stages[pb].concurrentApproaches
            ||(def.stages[s].concurrentApproaches==def.stages[pb].concurrentApproaches&&effective[s]>effective[pb])))pb=s;if(pb>=0)best=pb;}
    const double green=now-st.greenStartedSec;if(green+1e-9<m_minGreenSec)return;
    const double plannedGreenLimit=st.plannedGreenSec.value(current,m_maxGreenSec);
    const double serviceQuantumLimit=enabled(
        m_actuatedConfig,"capServiceQuantumAtBaseGreen",true)
        ? qMax(m_minGreenSec,double(def.stages[current].greenSec))
        : m_maxGreenSec;
    const double waveDemand = traffic.value(current).queueVeh
        + ev.value(current).predictedArrivalsVeh;
    const bool entryWaveDemand = waveDemand >= qMax(0.0, cfg(
        m_greenWaveConfig, "minimumDemandVeh", 1.0))
        && (isEntryGreenWaveGateway(index, current)
            || ev.value(current).greenWaveArrivalActive);
    const double greenWaveExtension = entryWaveDemand
        ? qMax(0.0, cfg(m_greenWaveConfig,
                        "maximumGreenExtensionSec", 8.0)) : 0.0;
    const double currentGreenLimit=qBound(m_minGreenSec,
        qMin(plannedGreenLimit+greenWaveExtension,serviceQuantumLimit),
        m_maxGreenSec);
    const double empty=qMax(0.0,cfg(m_actuatedConfig,"emptyApproachThresholdVeh",0.5));
    if(traffic[current].queueVeh<=empty){if(st.emptyStageStartedSec<0.0)st.emptyStageStartedSec=now;}else st.emptyStageStartedSec=-1.0;
    const double emptyElapsed=st.emptyStageStartedSec<0?0:qMax(0.0,now-st.emptyStageStartedSec);
    // If every downstream is marked congested, using `feasible` here leaves
    // no alternative and can hold one green forever. Rotate service by debt
    // across all stages until a downstream becomes available again.
    QVector<int> alternatives;for(int s:(allStagesBlocked?candidates:feasible))if(s!=current)alternatives.push_back(s);
    const bool currentFeasible=current>=0&&current<ev.size()&&!ev[current].spillbackBlocked;
    const bool starvation=enabled(m_actuatedConfig,"boundedStarvationEnabled",true);const double sq=qMax(0.0,cfg(m_actuatedConfig,"starvationQueueThresholdVeh",1.0));
    const int maxSkips=qMax(1,m_actuatedConfig.value("maximumConsecutiveStageSkips").toInt(3));int selected=-1;
    if(!currentFeasible&&!alternatives.isEmpty()){
        if(allStagesBlocked&&green+1e-9>=currentGreenLimit)selected=serviceDebtCandidate(index,alternatives,effective,traffic);
        else if(allStagesBlocked){/* keep serving until the bounded quantum */}
        else{selected=alternatives.first();for(int s:alternatives)if(effective[s]>effective[selected])selected=s;}}
    QVector<int> due;if(selected<0&&starvation){for(int s:alternatives)if(traffic[s].queueVeh>=sq&&st.redAgeSec[s]+1e-9>=hard)due.push_back(s);if(!due.isEmpty())selected=serviceDebtCandidate(index,due,effective,traffic);}
    if(selected<0&&starvation){due.clear();for(int s:alternatives)if(traffic[s].queueVeh>=sq&&st.stageSkipCount[s]>=maxSkips)due.push_back(s);if(!due.isEmpty())selected=serviceDebtCandidate(index,due,effective,traffic);}
    QVector<double> peak(traffic.size());for(int s=0;s<traffic.size();++s){peak[s]=traffic[s].queueVeh/qMax(1,def.stages[s].incomingLaneIds.size());for(double q:traffic[s].serviceableQueueByLane)peak[s]=qMax(peak[s],qMin(traffic[s].queueVeh,q));}
    if(selected<0&&starvation&&green<currentGreenLimit){QVector<int> u;const double first=qMax(m_minGreenSec,cfg(m_actuatedConfig,"firstServiceMinimumRedSec",20.0));for(int s:alternatives)if(!st.stageServiceCount[s]&&traffic[s].queueVeh>=sq&&st.redAgeSec[s]>=first)u.push_back(s);if(!u.isEmpty()){selected=u.first();for(int s:u)if(st.redAgeSec[s]>st.redAgeSec[selected]||(st.redAgeSec[s]==st.redAgeSec[selected]&&peak[s]>peak[selected]))selected=s;}}
    if(selected<0&&starvation){QVector<int> c;const double qt=qMax(sq,cfg(m_actuatedConfig,"criticalQueueThresholdVeh",8.0)),mr=qMax(m_minGreenSec,cfg(m_actuatedConfig,"criticalQueueMinimumRedSec",20.0)),margin=qMax(0.0,cfg(m_actuatedConfig,"criticalQueueMarginVeh",1.0));for(int s:alternatives)if(peak[s]>=qt&&st.redAgeSec[s]>=mr&&peak[s]>=peak[current]+margin)c.push_back(s);if(!c.isEmpty()){selected=c.first();for(int s:c)if(peak[s]>peak[selected])selected=s;}}
    if(selected<0&&green+1e-9>=currentGreenLimit){QVector<int> queued;for(int s:alternatives)if(traffic[s].queueVeh>empty)queued.push_back(s);if(!queued.isEmpty())selected=queued.contains(best)?best:serviceDebtCandidate(index,queued,effective,traffic);}
    if(selected<0&&green+1e-9>=m_maxGreenSec)selected=serviceDebtCandidate(index,alternatives,effective,traffic);
    const double gap=qMax(0.0,cfg(m_actuatedConfig,"gapOutHoldSec",0.0));if(selected<0&&traffic[current].queueVeh<=empty&&emptyElapsed+1e-9>=gap){QVector<int>w;for(int s:alternatives)if(traffic[s].queueVeh>empty)w.push_back(s);if(!w.isEmpty()){selected=w.contains(best)?best:w.first();if(!w.contains(best))for(int s:w)if(effective[s]>effective[selected])selected=s;}}
    const bool gapPending=selected<0&&traffic[current].queueVeh<=empty&&emptyElapsed+1e-9<gap&&std::any_of(alternatives.cbegin(),alternatives.cend(),[&](int s){return traffic[s].queueVeh>empty;});
    if(selected<0&&best!=current&&!feasible.isEmpty()&&!gapPending){const double relief=starvation?qMin(1.0,qMax(0.0,st.redAgeSec[best]-soft)/qMax(1.0,hard-soft)):0.0;const double switchRatio=green+1e-9<currentGreenLimit?qMax(m_switchRatio,cfg(m_actuatedConfig,"earlyPressureSwitchRatio",1.75)):m_switchRatio;double threshold=qMax(effective[current]*(1.0+(switchRatio-1.0)*(1.0-relief)),effective[current]+m_minimumGain*(1.0-relief));
        const bool continuity=enabled(m_actuatedConfig,"continuityProtectionEnabled",false)&&currentFeasible&&(traffic[current].queueVeh>=cfg(m_actuatedConfig,"continuityQueueThresholdVeh",3.0)||traffic[current].arrivalsVehPerMin>=cfg(m_actuatedConfig,"continuityArrivalThresholdVehPerMin",6.0))&&green+1e-9<qMin(currentGreenLimit,qMax(m_minGreenSec,cfg(m_actuatedConfig,"continuityProtectionMaxGreenSec",35.0)));
        if(continuity){const double loss=qMax(0.0,cfg(m_actuatedConfig,"clearanceLossPenaltyWeight",0.08))*(def.yellowSec+def.allRedSec)*(1.0+traffic[current].queueVeh/qMax(1,def.stages[current].incomingLaneIds.size()));threshold=qMax(threshold,effective[current]*qMax(m_switchRatio,cfg(m_actuatedConfig,"continuitySwitchRatio",1.28)));threshold=qMax(threshold,effective[current]+qMax(m_minimumGain,cfg(m_actuatedConfig,"continuityMinimumPressureGain",1.0))+loss);}if(effective[best]+1e-9>=threshold)selected=best;}
    if(selected>=0&&selected!=current)beginSwitch(index,selected,now);
}

void SignalEngine::update(double now,const TrafficAggregator& traffic)
{
    QMutexLocker locker(&m_mutex);
    if(!m_model)return;const QVector<QVector<StageTraffic>>& values=traffic.stages();const QVector<double> congestion=congestionByIntersection(values);double network=0.0;for(double v:congestion)network+=v;network/=qMax(1,congestion.size());
    QHash<QString,int> byId;for(int i=0;i<m_model->intersections().size();++i)byId.insert(m_model->intersections()[i].id,i);
    const double penalty=clampValue(cfg(m_terminalConfig,"downstreamCoordinationWeight",0.75),0.0,1.0),lo=qMax(0.01,cfg(m_terminalConfig,"minimumStageMultiplier",0.25)),hi=qMax(lo,cfg(m_terminalConfig,"maximumStageMultiplier",1.20)),smooth=clampValue(cfg(m_terminalConfig,"coordinationSmoothing",0.40),0.0,0.95);
    for(int i=0;i<m_states.size();++i)for(int s=0;s<m_model->intersections()[i].stages.size();++s){double down=0.0;for(const QString&id:m_model->intersections()[i].stages[s].downstreamIntersectionIds)down=qMax(down,congestion.value(byId.value(id,-1)));const double raw=clampValue(1.0-penalty*down,lo,hi),cached=m_states[i].terminalMultipliers.value(s,-1.0),old=cached<0.0?raw:cached;m_states[i].terminalMultipliers[s]=clampValue(smooth*old+(1.0-smooth)*raw,lo,hi);}
    for(int i=0;i<m_states.size();++i){const IntersectionDefinition&def=m_model->intersections()[i];if(!def.signalized||def.permanentGreen)continue;advanceTransition(i,now);IntersectionSignalState&st=m_states[i];for(int s=0;s<st.redAgeSec.size();++s)st.redAgeSec[s]=(st.transition==SignalTransition::Green&&s==st.activeStage)?0.0:qMax(0.0,now-qMax(0.0,st.stageRedStartedSec.value(s,now)));if(m_branch==ControlBranch::FixedTiming)updateFixed(i,now);else updateImproved(i,now,values.value(i),network,congestion);}
}

QString SignalEngine::colorForStage(int i,int s)const
{QMutexLocker locker(&m_mutex);if(!m_model||i<0||i>=m_states.size())return"R";const IntersectionDefinition&def=m_model->intersections()[i];if(!def.signalized)return"U";if(def.permanentGreen)return"G";const auto&st=m_states[i];if(s!=st.activeStage)return"R";return st.transition==SignalTransition::Green?"G":st.transition==SignalTransition::Yellow?"Y":"R";}
QString SignalEngine::colorForLane(qint64 laneId)const
{QMutexLocker locker(&m_mutex);if(!m_model)return"R";const QVector<StageRef> refs=m_model->controlledIncomingStagesByLane().value(laneId);if(refs.isEmpty())return"G";const QSet<QString> turns=m_model->turnTypesByLane().value(laneId);if(m_permissiveRightTurn&&turns.size()==1&&turns.contains("right"))return"G";QString result="R";for(const StageRef&r:refs){const QString c=colorForStage(r.intersection,r.stage);if(c=="G")return"G";if(c=="Y")result="Y";}return result;}
QString SignalEngine::activeStageId(int i)const
{QMutexLocker locker(&m_mutex);if(!m_model||i<0||i>=m_states.size())return QString();const IntersectionDefinition&def=m_model->intersections()[i];if(!def.signalized)return"UNSIGNALIZED_YIELD";if(def.permanentGreen)return"ALL_PHASES_GREEN";return def.stages.value(m_states[i].activeStage).id;}
QString SignalEngine::authorityOwner(int i)const
{QMutexLocker locker(&m_mutex);if(!m_model||i<0||i>=m_states.size())return QString();const IntersectionDefinition&def=m_model->intersections()[i];if(!def.signalized)return"LOCAL_YIELD";if(def.permanentGreen)return"PERMANENT_GREEN";return m_states[i].edgeIsLeader?"EDGE_RSU":"CLOUD_TERMINAL";}
double SignalEngine::waitForMovement(qint64 fromLinkId,qint64 toLinkId,double now)const
{
    QMutexLocker locker(&m_mutex);
    if(!m_model)return 0.0;
    QVector<StageRef> refs=m_model->movementStagesByLink().value(fromLinkId).value(toLinkId);
    if(refs.isEmpty())refs=m_model->incomingStagesByLink().value(fromLinkId);
    double answer=std::numeric_limits<double>::infinity();
    for(const StageRef&r:refs){
        if(r.intersection<0||r.intersection>=m_states.size())continue;
        const IntersectionSignalState&st=m_states[r.intersection];
        const IntersectionDefinition&def=m_model->intersections()[r.intersection];
        if(!def.signalized||def.permanentGreen){answer=0.0;continue;}
        if(m_branch==ControlBranch::FixedTiming){
            const StageDefinition stage=def.stages.value(r.stage);
            const double cycle=qMax(1,def.cycleSec);
            double position=std::fmod(now,cycle);if(position<0.0)position+=cycle;
            const double start=stage.startOffsetSec;
            const double greenEnd=start+stage.greenSec;
            const double wait=(position>=start&&position<greenEnd)?0.0:
                (position<start?start-position:cycle-position+start);
            answer=qMin(answer,wait);
            continue;
        }
        if(r.stage==st.activeStage&&st.transition==SignalTransition::Green){answer=0.0;continue;}
        const double elapsed=qMax(0.0,now-st.stateStartedSec);
        double nextWait=0.0;
        if(st.transition==SignalTransition::Yellow)
            nextWait=qMax(0.0,def.yellowSec-elapsed)+def.allRedSec;
        else if(st.transition==SignalTransition::AllRed)
            nextWait=qMax(0.0,def.allRedSec-elapsed);
        else
            nextWait=qMax(0.0,m_minGreenSec-qMax(0.0,now-st.greenStartedSec))
                +def.yellowSec+def.allRedSec;
        const double wait=r.stage==st.pendingStage?nextWait:
            nextWait+m_minGreenSec+def.yellowSec+def.allRedSec;
        answer=qMin(answer,wait);
    }
    return std::isfinite(answer)?answer:0.0;
}
int SignalEngine::allRedIntersectionCount()const
{QMutexLocker locker(&m_mutex);int n=0;if(!m_model)return n;for(int i=0;i<m_states.size();++i){const IntersectionDefinition&def=m_model->intersections()[i];if(def.signalized&&!def.permanentGreen&&m_states[i].transition==SignalTransition::AllRed)++n;}return n;}

int SignalEngine::entryGreenWaveGatewayStageCount() const
{ QMutexLocker locker(&m_mutex); return m_entryGreenWaveGatewayStageCount; }

int SignalEngine::entryGreenWaveCorridorStageCount() const
{ QMutexLocker locker(&m_mutex); return m_entryGreenWaveCorridorStageCount; }

int SignalEngine::entryGreenWaveArcCount() const
{ QMutexLocker locker(&m_mutex); return m_entryGreenWaveArcCount; }
