#include <cmath>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>
#include <QTextStream>

#include "core/ProjectModel.h"
#include "core/DemandEngine.h"
#include "core/MetricsRecorder.h"
#include "core/ReversibleMaxPressure.h"
#include "core/RoutingEngine.h"
#include "core/SignalEngine.h"
#include "core/SimulationRestartGuard.h"
#include "core/TrafficAggregator.h"

namespace {
bool check(bool condition, const QString& message)
{
    if (!condition) QTextStream(stderr) << "FAIL: " << message << Qt::endl;
    return condition;
}

bool verifyManualResetLifecycleGuard()
{
    SimulationRestartGuard guard;

    const ManualResetAction stoppingGap = guard.requestReset(true, false);
    const std::uint64_t firstGeneration = guard.generation();
    if (!check(stoppingGap == ManualResetAction::WaitForStopCallback,
               "manual reset started inside the native stopping gap")
        || !check(guard.isCurrent(firstGeneration),
                  "manual reset was not retained until afterStop")
        || !check(!guard.startRequested(),
                  "reset implicitly requested a new run"))
        return false;

    guard.requestStart();
    if (!check(guard.startRequested(),
               "explicit start was not retained during the stopping gap"))
        return false;

    guard.startAccepted();
    if (!check(!guard.pending() && !guard.startRequested(),
               "accepted manual-reset start remained pending"))
        return false;

    const ManualResetAction running = guard.requestReset(true, true);
    if (!check(running == ManualResetAction::RequestStop,
               "running manual reset did not request a full stop"))
        return false;

    const std::uint64_t cancelledGeneration = guard.generation();
    guard.cancel();
    const ManualResetAction stopped = guard.requestReset(false, false);
    return check(stopped == ManualResetAction::ReadyStopped,
                 "stopped manual reset did not remain stopped")
        && check(!guard.startRequested(),
                 "stopped manual reset scheduled an implicit start")
        && check(!guard.isCurrent(cancelledGeneration),
                 "cancelled reset timer remained current");
}

bool verifyReversiblePressureComponent()
{
    QJsonObject configuration;
    configuration["enabled"] = true;
    configuration["occupancyBackPressureVeh"] = 6.0;
    configuration["occupancyBackPressureExponent"] = 2.0;
    configuration["occupancyTrendBackPressureVeh"] = 8.0;
    configuration["maximumAdditionalBackPressureVeh"] = 5.0;
    configuration["pressureDirectionDeadbandVeh"] = 0.5;
    ReversibleMaxPressure component(configuration);

    const ReversiblePressureEvidence forward = component.evaluate(
        12.0, 2.0, 0.75, 0.20, 0.20, 0.0);
    if (!check(forward.direction == PressurePropagationDirection::Forward,
               "upstream demand did not produce forward pressure")
        || !check(forward.signedPressureVeh > 0.0
                  && qAbs(forward.usablePressureVeh
                          - forward.signedPressureVeh) < 1e-9,
                  "forward pressure was not serviceable"))
        return false;

    const ReversiblePressureEvidence reverse = component.evaluate(
        3.0, 8.0, 0.75, 0.75, 0.80, 0.05);
    if (!check(reverse.direction == PressurePropagationDirection::Reverse,
               "downstream congestion did not propagate reverse pressure")
        || !check(reverse.signedPressureVeh < 0.0
                  && reverse.usablePressureVeh == 0.0,
                  "reverse pressure was not clipped only at service scoring")
        || !check(!reverse.changesVehicleDirection
                  && !reverse.changesLaneDirection
                  && !reverse.changesLinkDirection,
                  "pressure propagation changed a physical direction"))
        return false;

    const ReversiblePressureEvidence bounded = component.evaluate(
        20.0, 0.0, 0.75, 1.0, 1.0, 1.0);
    if (!check(qAbs(bounded.occupancyBackPressureVeh
                    + bounded.occupancyTrendBackPressureVeh - 5.0) < 1e-9,
               "additional reverse pressure exceeded its bound"))
        return false;

    configuration["enabled"] = false;
    component.configure(configuration);
    const ReversiblePressureEvidence compatible = component.evaluate(
        10.0, 4.0, 0.75, 0.95, 1.0, 0.5);
    return check(qAbs(compatible.signedPressureVeh - 7.0) < 1e-9,
                 "disabled reversible pressure did not preserve V4 queue difference")
        && check(compatible.occupancyBackPressureVeh == 0.0
                 && compatible.occupancyTrendBackPressureVeh == 0.0,
                 "disabled reversible pressure kept additional backpressure");
}

bool verifyConfiguration(const ProjectModel& model)
{
    const QJsonObject signalConfig = model.scenario().value("signals").toObject();
    const QJsonObject runtime = model.scenario().value("runtimeControl").toObject();
    const QJsonObject actuated = runtime.value("actuatedMaxPressure").toObject();
    const QJsonObject authority = runtime.value("dynamicAuthority").toObject();
    const QJsonObject pressure = runtime.value("improvedMaxPressure").toObject();
    const QJsonObject reversiblePressure = pressure.value(
        "reversibleMaxPressure").toObject();
    const QJsonObject greenWave = pressure.value(
        "entryArterialGreenWave").toObject();
    const QJsonObject cav = model.scenario().value("cav").toObject();
    const QJsonObject laneBalancing = cav.value("laneBalancing").toObject();
    const QJsonObject generation =
        model.scenario().value("runtimeVehicleGeneration").toObject();
    const QJsonObject demand = model.scenario().value("demand").toObject();
    const QJsonArray disabledMovements = model.scenario().value("network")
        .toObject().value("disabledMovements").toArray();
    const QJsonObject topologyOverrides = model.scenario().value("network")
        .toObject().value("topologyOverrides").toObject();
    const QJsonArray mergedIntersections = signalConfig.value(
        "mergedIntersections").toArray();
    const QJsonArray permanentGreen = signalConfig.value(
        "permanentGreenIntersectionIds").toArray();
    return check(signalConfig.value("minGreenSec").toInt() == 12, "minGreenSec")
        && check(signalConfig.value("maxGreenSec").toInt() == 45, "maxGreenSec")
        && check(signalConfig.value("yellowSec").toInt() == 3, "yellowSec")
        && check(signalConfig.value("allRedSec").toInt() == 2, "allRedSec")
        && check(signalConfig.value("vehicleGreenReleaseMaximumImpulseAttempts")
                     .toInt() == 1,
                 "single bounded green release impulse")
        && check(signalConfig.value("pairOpposingPhysicalApproaches").toBool(),
                 "opposing physical approach pairing")
        && check(mergedIntersections.size() == 1,
                 "J253/J254 merge configuration")
        && check(permanentGreen.size() == 3
                 && permanentGreen.contains(QStringLiteral("J251"))
                 && permanentGreen.contains(QStringLiteral("J253"))
                 && permanentGreen.contains(QStringLiteral("J254")),
                 "J251 and J253-J254 permanent-green configuration")
        && check(topologyOverrides.value("removedLinkIds").toArray().size() == 4
                 && topologyOverrides.value("addedEdges").toArray().size() == 3,
                 "J253-J254 hand-edited topology override")
        && check(disabledMovements.size() == 1,
                 "J251 west-left movement closure count")
        && check(disabledMovements.first().toObject()
                     .value("intersectionId").toString() == QStringLiteral("J251")
                 && disabledMovements.first().toObject()
                     .value("turnType").toString() == QStringLiteral("left")
                 && disabledMovements.first().toObject()
                     .value("connectorId").toInt() == 9709
                 && disabledMovements.first().toObject()
                     .value("fromLinkId").toInt() == 5675
                 && disabledMovements.first().toObject()
                     .value("toLinkId").toInt() == 4452,
                 "J251 west-left movement closure identity")
        && check(actuated.value("decisionIntervalSec").toDouble() == 5.0,
                 "decisionIntervalSec")
        && check(!actuated.value("capServiceQuantumAtBaseGreen").toBool(),
                 "adaptive pressure green must not be capped by fixed timing")
        && check(actuated.value("earlyPressureSwitchRatio").toDouble() >= 1.5,
                 "early pressure switch protection")
        && check(actuated.value("continuityProtectionEnabled").toBool(),
                 "continuity protection")
        && check(authority.value("enabled").toBool(), "dynamic authority")
        && check(authority.value("uniqueSignalWriter").toBool(), "single writer")
        && check(pressure.value("hardSpillbackGateEnabled").toBool(),
                 "spillback gate")
        && check(pressure.value("compatibleParallelismEnabled").toBool(),
                 "compatible parallelism")
        && check(pressure.value("modelId").toString()
                     == QStringLiteral(
                         "REVERSIBLE_PRESSURE_CONTINUITY_STORAGE_CONSTRAINED_"
                         "CONFLICT_GRAPH_RSU_IMPROVED_MAX_PRESSURE_V5"),
                 "reversible-pressure improved max-pressure model id")
        && check(reversiblePressure.value("enabled").toBool(),
                 "reversible pressure propagation")
        && check(reversiblePressure.value("modelId").toString()
                     == ReversibleMaxPressure::defaultModelId(),
                 "reversible pressure component model id")
        && check(greenWave.value("enabled").toBool(),
                 "entry arterial green wave")
        && check(greenWave.value("outerBoundaryOnly").toBool(),
                 "green wave must originate at outer generators")
        && check(greenWave.value("maximumSignalDepth").toInt() >= 8,
                 "green wave does not cover enough downstream signals")
        && check(greenWave.value("entryGatewayPressureGain").toDouble() > 0.0
                 && greenWave.value("progressionPressureGain").toDouble() > 0.0,
                 "green wave pressure gain")
        && check(greenWave.value("maximumGreenExtensionSec").toDouble() > 0.0,
                 "entry green extension")
        && check(cav.value("dynamicRoutingEnabled").toBool(), "dynamic routing")
        && check(cav.value("replanAtEveryDecisionPoint").toBool(),
                 "dynamic reroute at every upstream decision point")
        && check(cav.value("routeSyncRetrySec").toDouble() >= 5.0,
                 "debounced native route synchronization retry")
        && check(cav.value("lockDecisionUntilIncomingLinkExit").toBool(),
                 "no route flapping inside one incoming link")
        && check(!cav.value("allowLateConnectorReplan").toBool(),
                 "no late route change inside stop-line approach")
        && check(laneBalancing.value("enabled").toBool(),
                 "lane balancing")
        && check(laneBalancing.value("controlRadiusBeforeStopLineM")
                     .toDouble() >= 300.0,
                 "early terminal lane alignment radius")
        && check(laneBalancing.value("laneChangeInitiationCutoffBeforeStopLineM")
                     .toDouble() >= 200.0,
                 "long lane change completion buffer before stop line")
        && check(laneBalancing.value("freezeDistanceBeforeStopLineM")
                     .toDouble() >= 50.0
                 && laneBalancing.value("freezeDistanceBeforeStopLineM")
                     .toDouble() <= 90.0,
                 "stop-line lane change freeze")
        && check(!laneBalancing.value("dismissEarlyLaneChangeInProtectedZone")
                      .toBool(),
                 "finish existing lane change instead of stop-line dismissal")
        && check(laneBalancing.value("queueLaneChangeProtectionEnabled")
                     .toBool(),
                 "queued approach lane-change protection")
        && check(laneBalancing.value("hardFreezeCurrentLaneInProtectedZone")
                     .toBool(),
                 "hard current-lane freeze in protected approach")
        && check(laneBalancing.value("nativeLaneChangeCallbackSteps")
                     .toInt() == 1,
                 "native lane-change callbacks run every calculation step")
        && check(laneBalancing.value("queueAnticipationFrontDistanceM")
                     .toDouble() >= 20.0
                 && laneBalancing.value("queueAnticipationFrontDistanceM")
                     .toDouble() <= 40.0,
                 "local stopped-leader anticipation without whole-link lock")
        && check(laneBalancing.value("queueLaneLockDistanceM").toDouble()
                     >= laneBalancing.value("freezeDistanceBeforeStopLineM")
                         .toDouble()
                 && laneBalancing.value("queueLaneLockDistanceM").toDouble()
                     <= 120.0,
                 "local queue lane-lock zone")
        && check(laneBalancing.value("minimumMandatoryLaneChangeSpeedMps")
                     .toDouble() >= 4.0,
                 "no mandatory lane change in standing queue")
        && check(laneBalancing.value("maximumMandatoryLaneChangeOccupancy")
                     .toDouble() <= 0.55,
                 "mandatory lane-change source occupancy guard")
        && check(laneBalancing.value("maximumMandatoryTargetLaneOccupancy")
                     .toDouble() <= 0.55,
                 "mandatory lane-change target occupancy guard")
        && check(laneBalancing.value("minimumMandatoryFrontGapM")
                     .toDouble() >= 12.0,
                 "mandatory lane-change front gap")
        && check(laneBalancing.value("minimumMandatoryRearGapM")
                     .toDouble() >= 10.0,
                 "mandatory lane-change rear gap")
        && check(laneBalancing.value("rearClosingTimeBufferSec")
                     .toDouble() >= 1.0,
                 "mandatory lane-change rear closing buffer")
        && check(laneBalancing.value(
                     "minimumLaneChangeStartIntervalSecPerLink")
                     .toDouble() >= 1.0,
                 "serialized lane-change start cadence")
        && check(laneBalancing.value("maximumConcurrentLaneChangesPerLink")
                     .toInt() == 1,
                 "single concurrent lane change per approach link")
        && check(laneBalancing.value("approachLaneChangeSafeDistanceM")
                     .toDouble() >= 12.0,
                 "approach lane-change safe distance")
        && check(laneBalancing.value("maximumRouteAlignmentAttemptsPerLink")
                     .toInt() == 1,
                 "single upstream route alignment attempt")
        && check(laneBalancing.value("maximumQueueBalanceAttemptsPerLink")
                     .toInt() == 0,
                 "optional lane balance disabled on saturated approaches")
        && check(laneBalancing.value("connectorReselectionIntervalSec")
                     .toDouble() >= 5.0,
                 "bounded upstream connector refresh cadence")
        && check(laneBalancing.value(
                     "connectorEmergencyReselectionMinDistanceM").toDouble()
                     >= laneBalancing.value(
                         "laneChangeInitiationCutoffBeforeStopLineM").toDouble(),
                 "connector spillback escape remains upstream of commitment")
        && check(laneBalancing.value("connectorEmergencyBlockOccupancy")
                     .toDouble()
                 > laneBalancing.value("connectorEmergencyReleaseOccupancy")
                     .toDouble(),
                 "connector spillback escape hysteresis")
        && check(laneBalancing.value("maximumConnectorReselectionsPerLink")
                     .toInt() == 1,
                 "single connector spillback escape per link")
        && check(laneBalancing.value("minimumOptionalLaneChangeSpeedMps")
                     .toDouble() >= 3.0,
                 "no optional lane change in standing queue")
        && check(laneBalancing.value("congestedLaneRecoveryOccupancy")
                     .toDouble() <= 0.60,
                 "early destination-preserving reroute on congested lanes")
        && check(signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("maximumDownstreamOccupancy").toDouble()
                 >= signalConfig.value("greenReleaseMaxDownstreamOccupancy").toDouble(),
                 "connector evacuation uses receiving-capacity threshold")
        && check(signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("protectedGreenStoppedThresholdSec").toDouble()
                 <= 1.0,
                 "protected single-approach connector clears promptly")
        && check(signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("protectedGreenClearSpeedMps").toDouble()
                 > signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("creepSpeedMps").toDouble(),
                 "protected connector clearance exceeds generic creep")
        && check(signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("maximumImpulseAttempts").toInt() >= 2
                 && signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("maximumImpulseAttempts").toInt() <= 5
                 && signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("retryIntervalSec").toDouble() >= 1.0,
                 "bounded spaced connector recovery retries")
        && check(signalConfig.value("connectorDeadlockRecovery").toObject()
                     .value("perIntersectionReleaseCooldownSec").toDouble()
                 >= 0.5,
                 "serialized connector deadlock recovery")
        && check(cav.value("laneDropPenaltySecPerLane").toDouble() > 0.0,
                 "lane-drop route penalty")
        && check(cav.value("narrowLinkCongestionPenaltySec").toDouble() > 0.0,
                 "dynamic narrow-link route penalty")
        && check(cav.value("movementLaneScarcityPenaltySec").toDouble() > 0.0,
                 "movement lane scarcity penalty")
        && check(demand.value("residentialVehiclesPerEntryPerHour").toInt() == 100,
                 "residential demand default")
        && check(generation.value("minimumSuccessfulSpawnHeadwaySec")
                     .toDouble() >= 1.0,
                 "boundary demand anti-burst headway");
}

bool verifyManifestMappings(const ProjectModel& model)
{
    int movementCount = 0;
    int expectedOffsetMismatchCount = 0;
    QHash<qint64, int> laneStageCounts;
    int j253Index = -1;
    int j254Index = -1;
    int j124Index = -1;
    int j251Index = -1;
    int pairedStageCount = 0;
    int concurrentStageCount = 0;
    for (int i = 0; i < model.intersections().size(); ++i) {
        const IntersectionDefinition& intersection = model.intersections()[i];
        if (intersection.id == "J253") j253Index = i;
        if (intersection.id == "J254") j254Index = i;
        if (intersection.id == "J124") j124Index = i;
        if (intersection.id == "J251") j251Index = i;
        if (!check(!intersection.stages.isEmpty(), intersection.id + " has no stages"))
            return false;
        int expectedOffset = 0;
        for (int s = 0; s < intersection.stages.size(); ++s) {
            const StageDefinition& stage = intersection.stages[s];
            if (stage.startOffsetSec != expectedOffset)
                ++expectedOffsetMismatchCount;
            expectedOffset += stage.greenSec + stage.yellowSec + stage.allRedSec;
            if (!check(stage.conflictFree || stage.permissiveOpposing,
                       intersection.id + "/" + stage.id
                           + " unclassified conflict"))
                return false;
            if (stage.opposingPaired) ++pairedStageCount;
            if (stage.concurrentApproaches > 1) ++concurrentStageCount;
            for (qint64 laneId : stage.incomingLaneIds)
                ++laneStageCounts[laneId];
            for (const auto& movement : stage.movementLinkPairs) {
                ++movementCount;
                bool found = false;
                for (const StageRef& ref : model.movementStagesByLink()
                         .value(movement.first).value(movement.second))
                    found = found || (ref.intersection == i && ref.stage == s);
                if (!check(found, QString("movement map missing %1->%2")
                           .arg(movement.first).arg(movement.second))) return false;
                if (!check(!model.movementLanesByLink().value(movement.first)
                               .value(movement.second).isEmpty(),
                           QString("movement lanes missing %1->%2")
                               .arg(movement.first).arg(movement.second))) return false;
            }
        }
        if (expectedOffset != intersection.cycleSec)
            ++expectedOffsetMismatchCount;
    }
    for (auto it = model.incomingStagesByLane().cbegin();
         it != model.incomingStagesByLane().cend(); ++it) {
        if (!check(laneStageCounts.value(it.key()) == 1,
                   QString("physical lane %1 belongs to multiple stages")
                       .arg(it.key()))) return false;
        const QHash<qint64, QString> targets =
            model.turnTypeByLaneTarget().value(it.key());
        if (!check(!targets.isEmpty(),
                   QString("signal lane %1 has no reachable movement")
                       .arg(it.key()))) return false;
        for (auto target = targets.cbegin(); target != targets.cend(); ++target) {
            const qint64 fromLink = model.laneLinkById().value(it.key());
            if (!check(model.movementLanesByLink().value(fromLink)
                           .value(target.key()).contains(it.key()),
                       QString("lane %1 cannot reach target %2")
                           .arg(it.key()).arg(target.key()))) return false;
        }
    }
    if (!check(j253Index < 0, "J253 must be merged into J254")) return false;
    if (!check(j254Index >= 0 && model.intersections()[j254Index].signalized,
               "merged J254 must remain signalized")) return false;
    const IntersectionDefinition& merged = model.intersections()[j254Index];
    if (!check(merged.permanentGreen,
               "merged J253-J254 must stay permanently green")) return false;
    if (!check(merged.mergedMemberIds.contains(QStringLiteral("J253")),
               "J254 merge membership")) return false;
    if (!check(model.controlledIncomingStagesByLane().contains(6071)
               && model.controlledIncomingStagesByLane().contains(4739)
               && model.controlledIncomingStagesByLane().contains(6202)
               && !model.controlledIncomingStagesByLane().contains(4737)
               && !model.controlledIncomingStagesByLane().contains(4738)
               && !model.controlledIncomingStagesByLane().contains(6072)
               && !model.controlledIncomingStagesByLane().contains(6203),
               "merged permanent-green stop-line lane binding")) return false;
    if (!check(!model.movementStagesByLink().value(5671).value(5674).isEmpty()
               && !model.movementStagesByLink().value(4432).value(4429).isEmpty()
               && !model.movementStagesByLink().value(4432).value(5726).isEmpty()
               && !model.movementStagesByLink().value(5725).value(4429).isEmpty(),
               "merged J253-J254 current movement mapping")) return false;
    if (!check(j124Index >= 0, "J124 exists")) return false;
    const IntersectionDefinition& j124 = model.intersections()[j124Index];
    if (!check(j124.stages.size() == 2,
               "J124 should use two opposing approach stages")) return false;
    for (const StageDefinition& stage : j124.stages)
        if (!check(stage.opposingPaired && stage.concurrentApproaches >= 2,
                   "J124 stage is not opposing paired")) return false;
    if (!check(j251Index >= 0, "J251 exists")) return false;
    const IntersectionDefinition& j251 = model.intersections()[j251Index];
    if (!check(j251.permanentGreen,
               "J251 must stay permanently green")) return false;
    bool j251WestStraightRetained = false;
    for (const StageDefinition& stage : j251.stages)
    {
        if (!check(!stage.permissiveOpposing
                       && stage.concurrentApproaches == 1,
                   "J251 must keep exclusive protected approaches"))
            return false;
        for (const auto& movement : stage.movementLinkPairs) {
            if (!check(movement != qMakePair<qint64, qint64>(5675, 4452),
                       "J251 west-left movement remains in a signal stage"))
                return false;
            if (movement == qMakePair<qint64, qint64>(5675, 6371))
                j251WestStraightRetained = true;
        }
    }
    if (!check(model.disabledConnectorIds().contains(9709)
               && model.isMovementDisabled(5675, 4452),
               "J251 west-left closure was not loaded")) return false;
    if (!check(model.movementStagesByLink().value(5675).value(4452).isEmpty()
               && model.movementLanesByLink().value(5675).value(4452).isEmpty()
               && !model.turnTypeByLaneTarget().value(6075).contains(4452),
               "J251 west-left movement remains executable")) return false;
    if (!check(j251WestStraightRetained
               && !model.movementStagesByLink().value(5675)
                       .value(6371).isEmpty()
               && model.movementLanesByLink().value(5675)
                       .value(6371).contains(6075)
               && model.turnTypeByLaneTarget().value(6075)
                       .value(6371) == QStringLiteral("straight"),
               "J251 west-east straight movement was removed accidentally"))
        return false;
    return check(movementCount > 1000, "too few mapped movements")
        && check(pairedStageCount >= 300,
                 "too few opposing approach pairs")
        && check(concurrentStageCount >= 500,
                 "too few concurrent approach stages")
        && check(expectedOffsetMismatchCount == 0,
                 "fixed timing offsets/cycle are not sequential")
        && check(model.expectedSignalLampCount() == 1592,
                 "expected signal lamp count");
}

bool verifyPermanentGreenIntersections(ProjectModel& model)
{
    TrafficAggregator traffic(&model);
    SignalEngine engine(&model);
    for (ControlBranch branch : {ControlBranch::FixedTiming,
                                 ControlBranch::ImprovedMaxPressure}) {
        engine.setBranch(branch);
        engine.reset(0.0);
        engine.update(3600.0, traffic);
        for (const QString& id : {QStringLiteral("J251"),
                                  QStringLiteral("J254")}) {
            int index = -1;
            for (int i = 0; i < model.intersections().size(); ++i)
                if (model.intersections()[i].id == id) { index = i; break; }
            if (!check(index >= 0, id + " permanent-green junction missing"))
                return false;
            const IntersectionDefinition& intersection =
                model.intersections()[index];
            const IntersectionSignalState& state = engine.states()[index];
            if (!check(intersection.signalized && intersection.permanentGreen
                       && state.transition == SignalTransition::Green
                       && state.activeStage == 0
                       && engine.activeStageId(index) == "ALL_PHASES_GREEN"
                       && engine.authorityOwner(index) == "PERMANENT_GREEN",
                       id + " permanent-green state changed"))
                return false;
            for (int stageIndex = 0;
                 stageIndex < intersection.stages.size(); ++stageIndex) {
                if (!check(engine.colorForStage(index, stageIndex) == "G",
                           id + " stage is not green"))
                    return false;
                const StageDefinition& stage = intersection.stages[stageIndex];
                for (qint64 laneId : stage.incomingLaneIds)
                    if (!check(engine.colorForLane(laneId) == "G",
                               id + " lane is not green"))
                        return false;
                for (const auto& movement : stage.movementLinkPairs)
                    if (!check(engine.waitForMovement(
                                   movement.first, movement.second, 3600.0) == 0.0,
                               id + " permanent-green movement has signal wait"))
                        return false;
            }
        }
    }
    return true;
}

bool verifySignalStateMachine(ProjectModel& model, ControlBranch branch)
{
    TrafficAggregator traffic(&model);
    SignalEngine engine(&model);
    engine.setBranch(branch);
    engine.reset(0.0);
    QVector<double> allRedStarted(model.intersections().size(), -1.0);
    for (int tick = 0; tick <= 2400; ++tick) {
        const double now = tick / 10.0;
        engine.update(now, traffic);
        const QVector<IntersectionSignalState>& states = engine.states();
        for (int i = 0; i < states.size(); ++i) {
            const IntersectionSignalState& state = states[i];
            if (!check(state.activeStage >= 0
                       && state.activeStage < model.intersections()[i].stages.size(),
                       model.intersections()[i].id + " invalid active stage")) return false;
            if (state.transition == SignalTransition::AllRed) {
                if (allRedStarted[i] < 0.0) allRedStarted[i] = now;
                if (!check(now - allRedStarted[i]
                           <= model.intersections()[i].allRedSec + 0.11,
                           model.intersections()[i].id + " all-red timeout")) return false;
            } else allRedStarted[i] = -1.0;
        }
    }
    return true;
}

bool verifyEntryArterialGreenWave(ProjectModel& model)
{
    SignalEngine engine(&model);
    QTextStream(stdout) << QString(
        "GREEN_WAVE gateways=%1 corridorStages=%2 progressionArcs=%3\n")
        .arg(engine.entryGreenWaveGatewayStageCount())
        .arg(engine.entryGreenWaveCorridorStageCount())
        .arg(engine.entryGreenWaveArcCount());
    return check(engine.entryGreenWaveGatewayStageCount() >= 18,
                 QString("too few entry green-wave gateways: %1")
                     .arg(engine.entryGreenWaveGatewayStageCount()))
        && check(engine.entryGreenWaveCorridorStageCount()
                     > engine.entryGreenWaveGatewayStageCount(),
                 QString("green wave did not propagate beyond entry gateways: %1")
                     .arg(engine.entryGreenWaveCorridorStageCount()))
        && check(engine.entryGreenWaveArcCount() >= 18,
                 QString("too few green-wave progression arcs: %1")
                     .arg(engine.entryGreenWaveArcCount()));
}

bool verifyPressureActuation(ProjectModel& model)
{
    int index = -1;
    for (int i = 0; i < model.intersections().size(); ++i)
        if (!model.intersections()[i].permanentGreen
            && model.intersections()[i].stages.size() == 2) {
            index = i;
            break;
        }
    if (!check(index >= 0, "no two-stage intersection")) return false;
    TrafficAggregator traffic(&model);
    QVector<QVector<StageTraffic>>& values =
        const_cast<QVector<QVector<StageTraffic>>&>(traffic.stages());
    StageTraffic& target = values[index][1];
    target.queueVeh = 30.0;
    target.vehicleCount = 30.0;
    target.arrivalsVehPerMin = 18.0;
    target.meanSpeedMps = 0.0;
    for (qint64 laneId : model.intersections()[index].stages[1].incomingLaneIds) {
        target.queueByLane.insert(laneId, 15.0);
        target.serviceableQueueByLane.insert(laneId, 15.0);
    }
    SignalEngine engine(&model);
    engine.setBranch(ControlBranch::ImprovedMaxPressure);
    engine.reset(0.0);
    engine.update(0.0, traffic);
    const double greenLimit = engine.states()[index].plannedGreenSec.value(0);
    const double switchTime = std::ceil(greenLimit / 5.0) * 5.0;
    engine.update(switchTime, traffic);
    const IntersectionSignalState pending = engine.states()[index];
    if (!check(pending.transition == SignalTransition::Yellow
               && pending.pendingStage == 1,
               model.intersections()[index].id + " pressure did not request stage 1"))
        return false;
    engine.update(switchTime + 3.0, traffic);
    if (!check(engine.states()[index].transition == SignalTransition::AllRed,
               "yellow did not enter all-red")) return false;
    engine.update(switchTime + 5.0, traffic);
    const IntersectionSignalState active = engine.states()[index];
    if (!check(active.transition == SignalTransition::Green
               && active.activeStage == 1, "all-red did not enter requested green"))
        return false;
    if (!check(active.authorityInitialized && active.authorityLeaseId > 0,
               "dynamic authority lease not active")) return false;
    for (qint64 laneId : model.intersections()[index].stages[1].incomingLaneIds)
        if (!check(engine.colorForLane(laneId) == "G", "green lane not released"))
            return false;
    return true;
}

bool verifyReversiblePressureIntegration(ProjectModel& model)
{
    int index = -1;
    for (int i = 0; i < model.intersections().size(); ++i) {
        if (!model.intersections()[i].permanentGreen
            && model.intersections()[i].stages.size() >= 2) {
            index = i;
            break;
        }
    }
    if (!check(index >= 0,
               "no signalized intersection for reversible-pressure audit"))
        return false;

    TrafficAggregator traffic(&model);
    QVector<QVector<StageTraffic>>& values =
        const_cast<QVector<QVector<StageTraffic>>&>(traffic.stages());
    StageTraffic& target = values[index][1];
    target.queueVeh = 3.0;
    target.vehicleCount = 3.0;
    target.arrivalsVehPerMin = 0.0;
    target.meanSpeedMps = 0.0;
    target.downstreamQueueVeh = 8.0;
    target.downstreamOccupancy = 0.80;

    SignalEngine engine(&model);
    engine.setBranch(ControlBranch::ImprovedMaxPressure);
    engine.reset(0.0);
    engine.update(0.0, traffic);
    const IntersectionSignalState state = engine.states().value(index);
    if (!check(state.lastReversiblePressure.size() > 1,
               "signal engine did not retain reversible-pressure evidence"))
        return false;
    const ReversiblePressureEvidence evidence =
        state.lastReversiblePressure.value(1);
    return check(evidence.direction == PressurePropagationDirection::Reverse,
                 "C++ signal score did not use downstream reverse pressure")
        && check(evidence.signedPressureVeh < 0.0
                 && evidence.usablePressureVeh == 0.0,
                 "C++ signal score did not use signed pressure")
        && check(!evidence.changesVehicleDirection
                 && !evidence.changesLaneDirection
                 && !evidence.changesLinkDirection,
                 "C++ pressure integration changed physical direction");
}

bool verifyAllBlockedSpillbackRecovery(ProjectModel& model)
{
    for (const QString& targetId : {QStringLiteral("J012"),
                                    QStringLiteral("J172")}) {
        int index = -1;
        for (int i = 0; i < model.intersections().size(); ++i)
            if (model.intersections()[i].id == targetId) { index = i; break; }
        if (!check(index >= 0, targetId + " missing from spillback audit"))
            return false;
        const int stageCount = model.intersections()[index].stages.size();
        if (!check(stageCount >= 2,
                   targetId + " has no alternative spillback stage"))
            return false;

        TrafficAggregator traffic(&model);
        QVector<QVector<StageTraffic>>& values =
            const_cast<QVector<QVector<StageTraffic>>&>(traffic.stages());
        for (int s = 0; s < stageCount; ++s) {
            values[index][s].downstreamOccupancy = 1.0;
            values[index][s].previousDownstreamOccupancy = 1.0;
            values[index][s].queueVeh = s == 0 ? 2.0 : 30.0 + s;
            for (qint64 laneId
                 : model.intersections()[index].stages[s].incomingLaneIds)
                values[index][s].serviceableQueueByLane.insert(
                    laneId, s == 0 ? 2.0 : 30.0 + s);
        }
        SignalEngine engine(&model);
        engine.setBranch(ControlBranch::ImprovedMaxPressure);
        engine.reset(0.0);
        engine.update(0.0, traffic);
        const int activeStage = engine.states()[index].activeStage;
        const double greenLimit = engine.states()[index]
            .plannedGreenSec.value(activeStage);
        engine.update(std::ceil(greenLimit / 5.0) * 5.0, traffic);
        const IntersectionSignalState state = engine.states()[index];
        if (!check(state.transition == SignalTransition::Yellow
                   && state.pendingStage >= 0
                   && state.pendingStage != activeStage,
                   targetId + " held one green while every exit was blocked"))
            return false;
    }
    return true;
}

bool verifyAdaptiveGreenPlanIsExecuted(ProjectModel& model)
{
    int index = -1;
    for (int i = 0; i < model.intersections().size(); ++i)
        if (!model.intersections()[i].permanentGreen
            && model.intersections()[i].stages.size() == 2) {
            index = i;
            break;
        }
    if (!check(index >= 0, "no two-stage intersection for green-plan test"))
        return false;
    TrafficAggregator traffic(&model);
    QVector<QVector<StageTraffic>>& values =
        const_cast<QVector<QVector<StageTraffic>>&>(traffic.stages());
    values[index][0].queueVeh = 30.0;
    values[index][1].queueVeh = 5.0;
    for (int s = 0; s < 2; ++s)
        for (qint64 laneId : model.intersections()[index].stages[s].incomingLaneIds)
            values[index][s].serviceableQueueByLane.insert(
                laneId, s == 0 ? 15.0 : 2.5);
    SignalEngine engine(&model);
    engine.setBranch(ControlBranch::ImprovedMaxPressure);
    engine.reset(0.0);
    engine.update(0.0, traffic);
    const QVector<double> initialPlan = engine.states()[index].plannedGreenSec;
    engine.update(5.0, traffic);
    if (!check(engine.states()[index].plannedGreenSec == initialPlan,
               "green plan changed more than once per cycle")) return false;
    const double limit = initialPlan.value(0);
    const double decisionTime = std::ceil(limit / 5.0) * 5.0;
    engine.update(decisionTime, traffic);
    const IntersectionSignalState state = engine.states()[index];
    return check(state.transition == SignalTransition::Yellow
                 && state.pendingStage == 1,
                 "planned green limit was not applied to signal switching");
}

bool verifyRouting(ProjectModel& model)
{
    TrafficAggregator traffic(&model);
    SignalEngine fixed(&model);
    fixed.setBranch(ControlBranch::FixedTiming);
    fixed.reset(0.0);
    fixed.update(0.0, traffic);
    RoutingEngine routing(&model);
    routing.rebuildShortestPathForests(traffic, &fixed, 0.0);
    if (!check(model.forbiddenExitLinkIds() == QSet<qint64>{5380},
               "unexpected unconfigured graph sinks")) return false;
    int positiveEdges = 0;
    bool j251WestLeftGraphEdgeFound = false;
    bool j251WestStraightGraphEdgeFound = false;
    QSet<QPair<qint64, qint64>> mergedCurrentEdges;
    for (const GraphEdge& edge : model.edges()) {
        if (edge.from == 5675 && edge.to == 4452)
            j251WestLeftGraphEdgeFound = true;
        if (edge.from == 5675 && edge.to == 6371)
            j251WestStraightGraphEdgeFound = true;
        if ((edge.from == 4432 && (edge.to == 4429 || edge.to == 5726))
            || (edge.from == 5671 && edge.to == 5674))
            mergedCurrentEdges.insert({edge.from, edge.to});
        if (!check(edge.from != 4430 && edge.from != 4431
                   && edge.from != 5672 && edge.from != 5673
                   && edge.to != 4430 && edge.to != 4431
                   && edge.to != 5672 && edge.to != 5673,
                   "deleted J253-J254 link remains in graph"))
            return false;
        const double cost = routing.edgeCostFor(edge.from, edge.to);
        if (!check(std::isfinite(cost) && cost > 0.0,
                   QString("invalid edge cost %1->%2").arg(edge.from).arg(edge.to)))
            return false;
        ++positiveEdges;
    }
    if (!check(positiveEdges == model.edges().size(), "edge cost coverage")) return false;
    if (!check(!j251WestLeftGraphEdgeFound && j251WestStraightGraphEdgeFound,
               "J251 graph closure removed the wrong movement")) return false;
    if (!check(mergedCurrentEdges.size() == 3,
               "J253-J254 current graph edges missing")) return false;
    int reachable = 0;
    int odAwareEntryMovements = 0;
    for (const TerminalDefinition& origin : model.terminals())
        for (const TerminalDefinition& destination : model.terminals()) {
            if (origin.id == destination.id) continue;
            const QVector<qint64> path = routing.plannedPathForTest(
                origin.entryLinkId, destination.exitLinkId);
            if (routing.nextHopFor(destination.exitLinkId, origin.entryLinkId) > 0
                && routing.freeFlowTravelTimeSec(origin.entryLinkId,
                                                  destination.exitLinkId) > 0.0
                && path.size() >= 2 && path.first() == origin.entryLinkId
                && path.last() == destination.exitLinkId) {
                ++reachable;
                const auto entryMovements = model.movementLanesByLink()
                    .value(origin.entryLinkId);
                if (!entryMovements.isEmpty()) {
                    if (!check(!entryMovements.value(path[1]).isEmpty(),
                               QString("terminal %1 has no OD-compatible spawn lane toward %2")
                                   .arg(origin.id).arg(destination.id)))
                        return false;
                    ++odAwareEntryMovements;
                }
            }
        }
    if (!check(reachable == 462, QString("terminal reachability only %1/462").arg(reachable)))
        return false;
    if (!check(odAwareEntryMovements > 0,
               "no terminal entry exposes OD-aware spawn lanes")) return false;
    int outerTerminalGateways = 0;
    for (const TerminalDefinition& terminal : model.terminals()) {
        if (terminal.group != QStringLiteral("OUTER_BOUNDARY")) continue;
        QSet<qint64> frontier{terminal.exitLinkId};
        QSet<qint64> visited = frontier;
        bool foundSignalizedGateway = false;
        for (int depth = 0; depth < 3 && !frontier.isEmpty()
             && !foundSignalizedGateway; ++depth) {
            QSet<qint64> predecessors;
            for (const GraphEdge& edge : model.edges()) {
                if (!frontier.contains(edge.to)) continue;
                const QVector<StageRef> refs = model.movementStagesByLink()
                    .value(edge.from).value(edge.to);
                for (const StageRef& ref : refs) {
                    if (ref.intersection >= 0
                        && model.intersections().value(ref.intersection).signalized) {
                        foundSignalizedGateway = true;
                        break;
                    }
                }
                if (!visited.contains(edge.from)) predecessors.insert(edge.from);
            }
            visited.unite(predecessors);
            frontier = predecessors;
        }
        if (!check(foundSignalizedGateway,
                   QString("outer terminal %1 has no signalized exit gateway")
                       .arg(terminal.id)))
            return false;
        ++outerTerminalGateways;
    }
    if (!check(outerTerminalGateways == 18,
               QString("outer terminal gateway coverage %1/18")
                   .arg(outerTerminalGateways)))
        return false;
    for (auto decisionIt = model.decisionsByLink().cbegin();
         decisionIt != model.decisionsByLink().cend(); ++decisionIt) {
        const LocalDecision& decision = decisionIt.value();
        if (decision.routingContextLinkId == 5675) {
            for (const LocalRouteOption& option : decision.routes)
                if (!check(option.nextLinkId != 4452,
                           "J251 west-left remains in local routing options"))
                    return false;
        }
        for (const TerminalDefinition& destination : model.terminals()) {
            const qint64 expected = routing.nextHopFor(
                destination.exitLinkId, decision.routingContextLinkId);
            const QVector<qint64> path = routing.plannedPathForTest(
                decision.routingContextLinkId, destination.exitLinkId);
            bool declared = false;
            for (const LocalRouteOption& option : decision.routes)
                declared = declared || option.nextLinkId == expected;
            if (!check(expected > 0 && declared && path.size() >= 2
                       && path.first() == decision.routingContextLinkId
                       && path[1] == expected
                       && path.last() == destination.exitLinkId,
                       QString("decision %1 has no OD-preserving route to %2")
                           .arg(decision.decisionLinkId).arg(destination.id)))
                return false;
            for (int linkIndex = 0; linkIndex + 1 < path.size(); ++linkIndex) {
                if (!check(routing.edgeCostFor(path[linkIndex],
                                               path[linkIndex + 1]) > 0.0,
                           QString("non-executable route edge %1->%2")
                               .arg(path[linkIndex]).arg(path[linkIndex + 1])))
                    return false;
                if (linkIndex > 0
                    && !check(!model.terminalIndexByExitLink().contains(
                                  path[linkIndex])
                              && !model.forbiddenExitLinkIds().contains(
                                  path[linkIndex]),
                              QString("route crosses wrong exit %1 before %2")
                                  .arg(path[linkIndex])
                                  .arg(destination.exitLinkId)))
                    return false;
            }
        }
    }
    const int destination = routing.deterministicDestinationForTest(0, 17);
    return check(destination >= 0 && destination != 0, "invalid deterministic OD")
        && check(destination == routing.deterministicDestinationForTest(0, 17),
                 "unstable deterministic OD")
        && check(routing.nextDestinationForOrigin(0)
                     == routing.deterministicDestinationForTest(0, 0),
                 "next runtime OD cannot be selected before spawn")
        && check(routing.nextDestinationForOrigin(0)
                     == routing.deterministicDestinationForTest(0, 0),
                 "pre-spawn OD selection advanced the departure sequence");
}

bool verifyDemandSequence(ProjectModel& model)
{
    const DemandEngine demand(&model);
    const QVector<int> cycle = demand.vehicleTypeCycleCodesForTest();
    if (!check(cycle.size() == 100, "vehicle type cycle size")) return false;
    QVector<int> specialPositions;
    for (int i = 0; i < cycle.size(); ++i)
        if (cycle[i] == 14) specialPositions.push_back(i);
    const QVector<int> expected{0, 2, 25, 55, 57, 79, 81, 89, 96, 97};
    return check(specialPositions == expected,
                 "vehicle type cycle differs from Python random.Random shuffle");
}

bool verifyPairedResultPersistence()
{
    QTemporaryDir temporary;
    if (!check(temporary.isValid(), "comparison temp directory")) return false;
    MetricsRecorder recorder(temporary.path(), nullptr);
    if (!check(recorder.nextComparisonRound() == 1,
               "initial comparison round allocation")) return false;

    NetworkTelemetry telemetry;
    telemetry.simulationTimeSec = 20.0;
    telemetry.generatedVehicles = 10;
    telemetry.runningVehicles = 8;
    telemetry.completedVehicles = 2;
    telemetry.meanSpeedKph = 31.5;
    telemetry.wrongExitBlockedVehicles = 2;
    telemetry.pendingGreenReleaseVehicles = 3;
    telemetry.connectorStallVehicles = 4;
    telemetry.routeMaintenanceFailureCount = 5;
    telemetry.stopLineLaneChangeDismissVehicles = 6;
    telemetry.greenReleaseImpulseCount = 7;
    telemetry.connectorDeadlockReleaseImpulseCount = 8;
    recorder.start(QStringLiteral("IMPROVED_MAX_PRESSURE"),
                   101, 202, 1, QStringLiteral("batch_test"), 1, 2);
    recorder.append(telemetry);
    QString error;
    if (!check(!recorder.finish(nullptr, &error).isEmpty(),
               QStringLiteral("save improved comparison result: %1").arg(error)))
        return false;

    recorder.start(QStringLiteral("FIXED_TIMING"),
                   101, 202, 1, QStringLiteral("batch_test"), 1, 2);
    recorder.append(telemetry);
    if (!check(!recorder.finish(nullptr, &error).isEmpty(),
               QStringLiteral("save fixed comparison result: %1").arg(error)))
        return false;
    if (!check(recorder.nextComparisonRound() == 2,
               "completed pair did not advance comparison round")) return false;

    QFile file(QDir(temporary.path()).filePath(
        QStringLiteral("result/round_001/FIXED_TIMING/experiment_result.json")));
    if (!check(file.open(QIODevice::ReadOnly), "open paired experiment result"))
        return false;
    const QJsonObject resultDocument = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject experiment = resultDocument
        .value(QStringLiteral("experiment")).toObject();
    const QJsonObject controlMethod = resultDocument
        .value(QStringLiteral("controlMethod")).toObject();
    const QJsonObject firstSample = resultDocument
        .value(QStringLiteral("series")).toArray().at(0).toObject();
    return check(experiment.value(QStringLiteral("vehicleGenerationRandomSeed")).toInt() == 101,
                 "paired vehicle seed metadata")
        && check(experiment.value(QStringLiteral("destinationChoiceRandomSeed")).toInt() == 202,
                 "paired destination seed metadata")
        && check(experiment.value(QStringLiteral("executionMode")).toString()
                     == QStringLiteral("AUTOMATIC_PAIRED_COMPARISON"),
                 "paired execution mode metadata")
        && check(controlMethod.value(QStringLiteral("modelId")).toString()
                     == QStringLiteral("FIXED_TIMING_BASELINE"),
                 "control method metadata")
        && check(firstSample.value(QStringLiteral("completedVehicles")).toInt() == 0,
                 "raw generated-minus-running leaked into completed vehicles")
        && check(firstSample.contains(QStringLiteral("removedVehicles")),
                 "removed vehicle field missing")
        && check(firstSample.value(QStringLiteral("wrongExitBlockedVehicles")).toInt() == 2,
                 "wrong-exit blockage diagnostic missing")
        && check(firstSample.value(QStringLiteral("pendingGreenReleaseVehicles")).toInt() == 3,
                 "green-release diagnostic missing")
        && check(firstSample.value(QStringLiteral("stopLineLaneChangeDismissVehicles")).toInt() == 6,
                 "stop-line lane-change diagnostic missing")
        && check(firstSample.value(QStringLiteral("connectorDeadlockReleaseImpulseCount")).toInt() == 8,
                 "connector deadlock recovery diagnostic missing");
}
}

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    if (argc < 2) { qCritical() << "usage: NativeCoreSelfTest <project-root>"; return 2; }
    ProjectModel model;
    QString error;
    if (!model.load(QString::fromLocal8Bit(argv[1]), &error)) {
        qCritical().noquote() << error; return 3;
    }
    if (!check(model.intersections().size() == 254, "intersection count")
        || !check(model.decisionsByLink().size() == 927, "decision count")
        || !check(model.terminals().size() == 22, "terminal count")
        || !check(model.placements().size() == 10000, "placement capacity")
        || !check(model.scenario().value("initialTraffic").toObject()
                      .value("targetVehicleCount").toInt() == 2000,
                  "default preload count")
        || !check(model.scenario().value("initialTraffic").toObject()
                      .value("maximumVehicleCount").toInt() == 10000,
                  "maximum preload count")
        || !check(model.links().size() >= 1200, "link count")
        || !check(model.edges().size() >= 2700, "edge count")) return 4;
    QSet<QString> preloadSlots;
    QSet<qint64> allPreloadLinks;
    QSet<qint64> defaultPreloadLinks;
    QSet<qint64> boundaryLinks;
    for (const TerminalDefinition& terminal : model.terminals()) {
        boundaryLinks.insert(terminal.entryLinkId);
        boundaryLinks.insert(terminal.exitLinkId);
    }
    bool preloadMetadataValid = true;
    for (int i = 0; i < model.placements().size(); ++i) {
        const InitialPlacement& placement = model.placements()[i];
        preloadSlots.insert(QStringLiteral("%1:%2:%3")
            .arg(placement.linkId)
            .arg(placement.laneNumber)
            .arg(qRound64(placement.distanceM * 1000.0)));
        allPreloadLinks.insert(placement.linkId);
        if (i < 2000) defaultPreloadLinks.insert(placement.linkId);
        preloadMetadataValid = preloadMetadataValid
            && !boundaryLinks.contains(placement.linkId)
            && !placement.originTerminalId.isEmpty()
            && !placement.destinationTerminalId.isEmpty()
            && placement.originTerminalId != placement.destinationTerminalId
            && placement.destinationExitLinkId > 0;
    }
    if (!check(preloadSlots.size() == model.placements().size(),
               "preload positions must be unique")
        || !check(defaultPreloadLinks == allPreloadLinks,
                  "default preload must cover every eligible preload link")
        || !check(allPreloadLinks.size() >= 900,
                  "preload must be distributed across the whole network")
        || !check(preloadMetadataValid,
                  "preload must exclude boundary links and keep valid OD metadata")) return 4;
    if (!verifyManualResetLifecycleGuard()) return 19;
    if (!verifyConfiguration(model)) return 5;
    if (!verifyManifestMappings(model)) return 6;
    if (!verifySignalStateMachine(model, ControlBranch::FixedTiming)) return 7;
    if (!verifySignalStateMachine(model, ControlBranch::ImprovedMaxPressure)) return 8;
    if (!verifyPermanentGreenIntersections(model)) return 9;
    if (!verifyEntryArterialGreenWave(model)) return 10;
    if (!verifyPressureActuation(model)) return 11;
    if (!verifyReversiblePressureComponent()) return 17;
    if (!verifyReversiblePressureIntegration(model)) return 18;
    if (!verifyAllBlockedSpillbackRecovery(model)) return 12;
    if (!verifyAdaptiveGreenPlanIsExecuted(model)) return 13;
    if (!verifyRouting(model)) return 14;
    if (!verifyDemandSequence(model)) return 15;
    if (!verifyPairedResultPersistence()) return 16;
    qInfo().noquote() << QString(
        "PASS intersections=%1 decisions=%2 terminals=%3 placements=%4 signalLanes=%5")
        .arg(model.intersections().size()).arg(model.decisionsByLink().size())
        .arg(model.terminals().size()).arg(model.placements().size())
        .arg(model.incomingStagesByLane().size());
    return 0;
}
