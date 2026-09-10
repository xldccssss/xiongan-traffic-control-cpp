#include "MySimulator.h"

#include <limits>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QSet>
#include <QThread>
#include <QTimer>

#include "IDecisionPoint.h"
#include "IDispatchPoint.h"
#include "IConnector.h"
#include "UnitChange.h"
#include "ILane.h"
#include "ILaneObject.h"
#include "ILaneConnector.h"
#include "IRouting.h"
#include "ILink.h"
#include "Plugin/StepsPerCall.h"
#include "isignallamp.h"
#include "ivehicle.h"
#include "ivehicledriving.h"
#include "netinterface.h"
#include "simuinterface.h"
#include "tessinterface.h"

MySimulator::MySimulator(ProjectModel* model, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_demand(model)
    , m_routing(model)
    , m_laneBalancer(model)
    , m_signals(model)
    , m_traffic(model)
    , m_metrics(model ? model->root() : QString(), model)
{
    if (m_model) {
        m_requestedBranch = m_signals.branch();
        const QJsonObject randomSeeds = m_model->scenario().value("randomSeeds").toObject();
        m_requestedVehicleSeed = qBound(
            0, randomSeeds.value("vehicleGeneration").toInt(20260822), 2147483647);
        m_requestedDestinationSeed = qBound(
            0, randomSeeds.value("destinationChoice").toInt(20260823), 2147483647);
        m_requestedPreloadCount = qBound(
            0,
            m_model->scenario().value("initialTraffic").toObject()
                .value("targetVehicleCount").toInt(m_model->placements().size()),
            m_model->placements().size());
        const QJsonObject visual = m_model->scenario()
            .value("dataVisualizationPanel").toObject();
        m_trajectoryRetentionPoints = qBound(
            5, visual.value("trajectoryRetentionPoints").toInt(30), 120);
        m_maximumDisplayedVehicles = qBound(
            20, visual.value("maximumDisplayedVehicles").toInt(400), 800);
    }
}

QString MySimulator::algorithmName() const
{
    return m_signals.branch() == ControlBranch::FixedTiming
        ? "FIXED_TIMING" : "IMPROVED_MAX_PRESSURE";
}

bool MySimulator::runtimeSmokeTestRequested() const
{
    const QCoreApplication* application = QCoreApplication::instance();
    return application && application->arguments().contains(
        QStringLiteral("--runtime-smoke-test"));
}

double MySimulator::runtimeSmokeTestDurationSec() const
{
    const QCoreApplication* application = QCoreApplication::instance();
    if (!application) return 6.0;
    const QString prefix = QStringLiteral("--runtime-smoke-duration=");
    for (const QString& argument : application->arguments()) {
        if (!argument.startsWith(prefix)) continue;
        bool ok = false;
        const double seconds = argument.mid(prefix.size()).toDouble(&ok);
        if (ok) return qBound(6.0, seconds, 600.0);
    }
    return 6.0;
}

int MySimulator::runtimeSmokeTargetMultiple() const
{
    const QCoreApplication* application = QCoreApplication::instance();
    if (!application) return m_targetMultiple;
    const QString prefix = QStringLiteral("--runtime-smoke-target-multiple=");
    for (const QString& argument : application->arguments()) {
        if (!argument.startsWith(prefix)) continue;
        bool ok = false;
        const int multiple = argument.mid(prefix.size()).toInt(&ok);
        if (ok) return qBound(1, multiple, 50);
    }
    return m_targetMultiple;
}

void MySimulator::finishRuntimeSmokeTest(bool success, const QString& error)
{
    if (!runtimeSmokeTestRequested() || m_runtimeSmokeCompleted || !m_model) return;
    m_runtimeSmokeCompleted = true;
    const NetworkTelemetry telemetry = m_traffic.telemetry();
    QJsonObject report;
    report.insert(QStringLiteral("timestamp"),
                  QDateTime::currentDateTime().toString(Qt::ISODate));
    report.insert(QStringLiteral("success"), success);
    report.insert(QStringLiteral("error"), error);
    report.insert(QStringLiteral("simulationTimeSec"), m_lastStepSec);
    report.insert(QStringLiteral("algorithm"), algorithmName());
    report.insert(QStringLiteral("signalLampCount"), m_signalLamps.size());
    report.insert(QStringLiteral("expectedSignalLampCount"),
                  m_model->expectedSignalLampCount());
    report.insert(QStringLiteral("signalReadbackAbnormalIntersections"),
                  m_signalReadbackAbnormalIntersections);
    report.insert(QStringLiteral("recoveredMovementCount"),
                  m_recoveredMovementCount);
    report.insert(QStringLiteral("greenReleaseImpulseCount"),
                  m_greenReleaseImpulseCount);
    report.insert(QStringLiteral("connectorDeadlockReleaseImpulseCount"),
                  m_connectorDeadlockReleaseImpulseCount);
    report.insert(QStringLiteral("stopLineLaneChangeDismissVehicles"),
                  m_stopLineLaneChangeDismissVehicleIds.size());
    report.insert(QStringLiteral("wrongExitBlockedVehicles"),
                  m_wrongExitBlockedVehicleIds.size());
    report.insert(QStringLiteral("connectorStallVehicles"),
                  m_connectorStallDetails.size());
    report.insert(QStringLiteral("routeMaintenanceFailureCount"),
                  m_routeMaintenanceFailures.size());
    double maximumPendingGreenReleaseSec = 0.0;
    for (double startedSec : m_greenStoppedSinceSec)
        maximumPendingGreenReleaseSec = qMax(
            maximumPendingGreenReleaseSec, m_lastStepSec - startedSec);
    report.insert(QStringLiteral("pendingGreenReleaseVehicles"),
                  m_greenStoppedSinceSec.size());
    report.insert(QStringLiteral("maximumPendingGreenReleaseSec"),
                  maximumPendingGreenReleaseSec);
    report.insert(QStringLiteral("maximumObservedGreenReleaseWaitSec"),
                  m_maximumGreenReleaseWaitSec);
    report.insert(QStringLiteral("wrongDestinationVehicles"),
                  m_metrics.wrongDestinationVehicleCount());
    report.insert(QStringLiteral("nonTerminalDisappearances"),
                  m_metrics.nonTerminalDisappearanceCount());
    QJsonArray pendingGreenReleaseDetails;
    for (auto it = m_greenStoppedSinceSec.cbegin();
         it != m_greenStoppedSinceSec.cend(); ++it) {
        QVariantMap detail = m_greenPendingDetails.value(it.key());
        detail.insert(QStringLiteral("vehicleId"), it.key());
        detail.insert(QStringLiteral("pendingSec"), m_lastStepSec - it.value());
        pendingGreenReleaseDetails.append(QJsonObject::fromVariantMap(detail));
    }
    report.insert(QStringLiteral("pendingGreenReleaseDetails"),
                  pendingGreenReleaseDetails);
    QJsonArray connectorStallDetails;
    for (auto it = m_connectorStallDetails.cbegin();
         it != m_connectorStallDetails.cend(); ++it) {
        QVariantMap detail = it.value();
        detail.insert(QStringLiteral("vehicleId"), it.key());
        connectorStallDetails.append(QJsonObject::fromVariantMap(detail));
    }
    report.insert(QStringLiteral("connectorStallVehicles"),
                  m_connectorStallDetails.size());
    report.insert(QStringLiteral("connectorStallDetails"), connectorStallDetails);
    report.insert(QStringLiteral("permissiveRightYieldVehicles"),
                  m_permissiveRightYieldVehicleIds.size());
    report.insert(QStringLiteral("wideningCorridorCount"),
                  m_laneBalancer.wideningCorridorCount());
    report.insert(QStringLiteral("wideningUnresolvedCount"),
                  m_laneBalancer.wideningUnresolvedCount());
    report.insert(QStringLiteral("preloadCreated"), m_demand.preloadCreated());
    report.insert(QStringLiteral("preloadTarget"), m_demand.preloadTarget());
    report.insert(QStringLiteral("vehiclesWithFixedOd"),
                  m_routing.vehicleCountWithOd());
    report.insert(QStringLiteral("routeDecisionCount"),
                  static_cast<qint64>(m_routing.routeDecisionCount()));
    report.insert(QStringLiteral("routeFallbackCount"),
                  static_cast<qint64>(m_routing.routeFallbackCount()));
    report.insert(QStringLiteral("nativeRouteSyncCount"),
                  static_cast<qint64>(m_routing.nativeRouteSyncCount()));
    report.insert(QStringLiteral("routeMaintenanceFailureCount"),
                  m_routeMaintenanceFailures.size());
    report.insert(QStringLiteral("wrongExitBlockedVehicles"),
                  m_wrongExitBlockedVehicleIds.size());
    QJsonObject routeMaintenanceFailuresByDecision;
    QJsonArray routeMaintenanceFailureExamples;
    for (const QString& key : m_routeMaintenanceFailures) {
        const QString owner = key.section(':', 1, 1);
        routeMaintenanceFailuresByDecision[owner] =
            routeMaintenanceFailuresByDecision.value(owner).toInt() + 1;
        if (routeMaintenanceFailureExamples.size() < 50)
            routeMaintenanceFailureExamples.append(key);
    }
    report.insert(QStringLiteral("routeMaintenanceFailuresByDecision"),
                  routeMaintenanceFailuresByDecision);
    report.insert(QStringLiteral("routeMaintenanceFailureExamples"),
                  routeMaintenanceFailureExamples);
    report.insert(QStringLiteral("runningVehicles"), telemetry.runningVehicles);
    report.insert(QStringLiteral("generatedVehicles"), telemetry.generatedVehicles);
    report.insert(QStringLiteral("completedVehicles"),
                  m_metrics.eligibleCompletedVehicleCount());
    report.insert(QStringLiteral("removedVehicles"), telemetry.removedVehicles);
    report.insert(QStringLiteral("queuedVehicles"), telemetry.queuedVehicles);
    report.insert(QStringLiteral("totalQueueVeh"), telemetry.totalQueueVeh);
    report.insert(QStringLiteral("sampledMeanSpeedKph"), telemetry.meanSpeedKph);
    report.insert(QStringLiteral("kpis"), m_metrics.performanceSnapshot());
    report.insert(QStringLiteral("measuredActualMultiple"), m_actualMultiple);
    report.insert(QStringLiteral("targetMultiple"), m_targetMultiple);
    report.insert(QStringLiteral("schedulerMultiple"), m_schedulerMultiple);
    report.insert(QStringLiteral("nativeThreadCount"), m_nativeThreadCount);
    report.insert(QStringLiteral("speedCapacityLimited"), m_speedCapacityLimited);
    qint64 totalStageServiceCount = 0;
    qint64 totalAuthoritySwitchCount = 0;
    for (const IntersectionSignalState& state : m_signals.states()) {
        for (int count : state.stageServiceCount)
            totalStageServiceCount += qMax(0, count);
        totalAuthoritySwitchCount += qMax<qint64>(0, state.authoritySwitchCount);
    }
    report.insert(QStringLiteral("totalStageServiceCount"),
                  totalStageServiceCount);
    report.insert(QStringLiteral("totalAuthoritySwitchCount"),
                  totalAuthoritySwitchCount);
    struct QueueDiagnostic {
        double queue = 0.0;
        QJsonObject value;
    };
    QVector<QueueDiagnostic> queueDiagnostics;
    const QVector<QVector<StageTraffic>>& stageTraffic = m_traffic.stages();
    const QVector<IntersectionSignalState>& signalStates = m_signals.states();
    for (int i = 0; i < m_model->intersections().size(); ++i) {
        const IntersectionDefinition& intersection = m_model->intersections()[i];
        const IntersectionSignalState state = signalStates.value(i);
        for (int s = 0; s < intersection.stages.size(); ++s) {
            const StageTraffic traffic = stageTraffic.value(i).value(s);
            QueueDiagnostic diagnostic;
            diagnostic.queue = traffic.queueVeh;
            diagnostic.value.insert(QStringLiteral("intersectionId"), intersection.id);
            diagnostic.value.insert(QStringLiteral("stageId"), intersection.stages[s].id);
            diagnostic.value.insert(QStringLiteral("queueVeh"), traffic.queueVeh);
            diagnostic.value.insert(QStringLiteral("downstreamQueueVeh"),
                                    traffic.downstreamQueueVeh);
            diagnostic.value.insert(QStringLiteral("downstreamOccupancy"),
                                    traffic.downstreamOccupancy);
            diagnostic.value.insert(QStringLiteral("active"),
                                    state.activeStage == s);
            diagnostic.value.insert(QStringLiteral("redAgeSec"),
                                    state.redAgeSec.value(s));
            diagnostic.value.insert(QStringLiteral("plannedGreenSec"),
                                    state.plannedGreenSec.value(s));
            diagnostic.value.insert(QStringLiteral("serviceCount"),
                                    state.stageServiceCount.value(s));
            diagnostic.value.insert(QStringLiteral("score"),
                                    state.lastEffectiveScores.value(s));
            queueDiagnostics.push_back(diagnostic);
        }
    }
    std::sort(queueDiagnostics.begin(), queueDiagnostics.end(),
              [](const QueueDiagnostic& a, const QueueDiagnostic& b) {
                  return a.queue > b.queue;
              });
    QJsonArray highestQueueStages;
    for (int i = 0; i < qMin(20, queueDiagnostics.size()); ++i)
        highestQueueStages.append(queueDiagnostics[i].value);
    report.insert(QStringLiteral("highestQueueStages"), highestQueueStages);
    int stageCount = 0;
    int concurrentStageCount = 0;
    int opposingPairedStageCount = 0;
    int permissiveOpposingStageCount = 0;
    for (const IntersectionDefinition& intersection : m_model->intersections()) {
        stageCount += intersection.stages.size();
        for (const StageDefinition& stage : intersection.stages) {
            if (stage.concurrentApproaches > 1) ++concurrentStageCount;
            if (stage.opposingPaired) ++opposingPairedStageCount;
            if (stage.permissiveOpposing) ++permissiveOpposingStageCount;
        }
    }
    report.insert(QStringLiteral("signalStageCount"), stageCount);
    report.insert(QStringLiteral("concurrentSignalStageCount"),
                  concurrentStageCount);
    report.insert(QStringLiteral("opposingPairedStageCount"),
                  opposingPairedStageCount);
    report.insert(QStringLiteral("permissiveOpposingStageCount"),
                  permissiveOpposingStageCount);
    QFile file(QDir(m_model->root()).filePath(
        QStringLiteral("generated/native_runtime_smoke_report.json")));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
}

void MySimulator::beforeStart(bool& keepOn)
{
    keepOn = m_model != nullptr;
    if (!keepOn) return;
    SimuInterface* simulation = gpTessInterface->simuInterface();
    if (runtimeSmokeTestRequested()) {
        m_targetMultiple = runtimeSmokeTargetMultiple();
        const QString prefix = QStringLiteral("--runtime-smoke-branch=");
        for (const QString& argument : QCoreApplication::arguments()) {
            if (!argument.startsWith(prefix)) continue;
            const QString branch = argument.mid(prefix.size()).trimmed().toUpper();
            if (branch == "FIXED_TIMING")
                m_requestedBranch = ControlBranch::FixedTiming;
            else if (branch == "IMPROVED_MAX_PRESSURE")
                m_requestedBranch = ControlBranch::ImprovedMaxPressure;
        }
    }
    simulation->setRandSeed(m_requestedVehicleSeed);
    simulation->setSimuAccuracy(10); // 10 calculations/s = 100 ms native update.
    const int laneChangeCallbackSteps = qMax(1, m_model->scenario()
        .value("cav").toObject().value("laneBalancing").toObject()
        .value("nativeLaneChangeCallbackSteps").toInt(1));
    // reCalcDismissChangeLane has no per-vehicle setter in the V4.1.3 SDK.
    // Run all preventive callbacks on every native calculation step so a new
    // manoeuvre is rejected before it passes the cancellable first third.
    gStepsPerCall.Steps_calcLimitedLaneNumber = laneChangeCallbackSteps;
    gStepsPerCall.Steps_calcChangeLaneSafeDist = laneChangeCallbackSteps;
    gStepsPerCall.Steps_reCalcDismissChangeLane = laneChangeCallbackSteps;
    const QJsonObject efficiency = m_model->scenario()
        .value("simulationPerformance").toObject()
        .value("visibleEfficiency").toObject();
    const int minimumThreads = qMax(1,
        efficiency.value("minimumThreadCount").toInt(2));
    const int maximumThreads = qMax(minimumThreads,
        efficiency.value("maximumThreadCount").toInt(8));
    const int configuredThreads = efficiency.value("threadCount").toInt(4);
    const int automaticThreads = qMax(1, QThread::idealThreadCount()
        - qMax(0, efficiency.value("reserveLogicalCores").toInt(0)));
    m_nativeThreadCount = qBound(minimumThreads,
        configuredThreads > 0 ? configuredThreads : automaticThreads,
        maximumThreads);
    simulation->setThreadCount(m_nativeThreadCount);
    m_schedulerMultiple = m_targetMultiple;
    simulation->setAcceMultiples(m_schedulerMultiple);
    simulation->setSimuIntervalScheming((m_durationSec + 15) * 1000L);
    simulation->setIsRecordTrace(false);
    simulation->setIsRecordPedestrianTrace(false);
    simulation->setIsRecordSignalLampStatus(false);

    // This process is the sole demand owner. Existing dispatch intervals are
    // disabled only in memory; the .tess master and C++ runtime copy are never saved.
    for (IDispatchPoint* point : gpTessInterface->netInterface()->dispatchPoints()) {
        if (point) {
            point->removeDispatchIntervals();
            point->setDynaModified(true);
        }
    }
    m_demand.setRandomSeed(m_requestedVehicleSeed);
    m_demand.setPreloadLimit(m_requestedPreloadCount);
    m_routing.setDestinationSeed(m_requestedDestinationSeed);
    m_demand.reset();
    m_routing.reset();
    m_laneBalancer.reset();
    const bool lanePolicyReady = m_laneBalancer.configure(
        gpTessInterface->netInterface());
    if (!lanePolicyReady || m_laneBalancer.wideningUnresolvedCount() > 0) {
        keepOn = false;
        emit eventGenerated(QStringLiteral(
            "[ERROR] 展宽过渡车道校验未通过：走廊=%1，未连通转向=%2，本轮禁止启动。")
            .arg(m_laneBalancer.wideningCorridorCount())
            .arg(m_laneBalancer.wideningUnresolvedCount()));
        finishRuntimeSmokeTest(false, QStringLiteral("widening lane policy validation failed"));
        if (runtimeSmokeTestRequested())
            QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        if (m_comparisonActive)
            finishComparisonBatch(QStringLiteral("自动对照已终止：展宽车道校验失败"));
        return;
    }
    m_traffic.reset();
    // A branch selection made during the previous run becomes authoritative
    // only at this run boundary, so one result file never mixes algorithms.
    m_signals.setBranch(m_requestedBranch);
    m_signals.reset(0.0);
    m_routing.rebuildShortestPathForests(m_traffic, &m_signals, 0.0);
    m_lastStepSec = 0.0;
    m_lastAggregateSec = -1e9;
    m_lastRouteRebuildSec = -1e9;
    m_lastTelemetrySec = -1e9;
    m_lastSignalReadbackSec = -1e9;
    m_lastSignalWarningSec = -1e9;
    m_signalReadbackTimeSec = 0.0;
    m_telemetrySampleCount = 0;
    m_signalReadbackAbnormalIntersections = 0;
    m_expectedSignalColorByLampId.clear();
    m_signalColorStableSinceSec.clear();
    m_trajectories.clear();
    m_lastVehicleLinkIds.clear();
    m_signalHardStoppedVehicleIds.clear();
    m_greenStoppedSinceSec.clear();
    m_greenReleaseLastImpulseSec.clear();
    m_greenReleaseImpulseAttempts.clear();
    m_greenPendingDetails.clear();
    m_connectorStallDetails.clear();
    m_connectorDeadlockRecoveryVehicleIds.clear();
    m_connectorDeadlockLastImpulseSec.clear();
    m_connectorDeadlockImpulseAttempts.clear();
    m_connectorDeadlockLastIntersectionReleaseSec.clear();
    m_lockedVehicleConnectorIds.clear();
    m_lockedVehicleConnectorFromLinkIds.clear();
    m_lockedVehicleConnectorSelectedAtSec.clear();
    m_lockedVehicleConnectorReselections.clear();
    m_lastLaneChangeStartSecByLink.clear();
    m_permissiveRightYieldVehicleIds.clear();
    m_wrongExitBlockedVehicleIds.clear();
    m_stopLineLaneChangeDismissVehicleIds.clear();
    m_routeMaintenanceFailures.clear();
    m_recoveredMovementCount = 0;
    m_greenReleaseImpulseCount = 0;
    m_connectorDeadlockReleaseImpulseCount = 0;
    m_maximumGreenReleaseWaitSec = 0.0;
    m_speedMeasurements.clear();
    m_actualMultiple = 0.0;
    m_speedIntegralError = 0.0;
    m_underTargetCount = 0;
    m_capacityLimitCount = 0;
    m_speedCapacityLimited = false;
    m_lastSpeedAdjustWallSec = -1e9;
    m_speedClock.restart();
    bindSignalLamps();
    if (m_signalLamps.size() != m_model->expectedSignalLampCount()) {
        keepOn = false;
        emit eventGenerated(QStringLiteral(
            "[ERROR] 信号灯完整性检查未通过：已绑定%1/%2个灯头，本轮禁止启动。")
            .arg(m_signalLamps.size()).arg(m_model->expectedSignalLampCount()));
        finishRuntimeSmokeTest(false, QStringLiteral("signal lamp integrity validation failed"));
        if (runtimeSmokeTestRequested())
            QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        if (m_comparisonActive)
            finishComparisonBatch(QStringLiteral("自动对照已终止：信号灯完整性校验失败"));
        return;
    }
    for (const auto& entry : m_signalLamps)
        entry.first->setLampColor(m_signals.colorForLane(entry.second));
    emit viewResetRequested();
    emit eventGenerated(QStringLiteral(
        "[%1] 原生C++本轮就绪：算法=%2，信号灯=%3，决策点=%4，预载车=%5，种子=%6/%7")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), algorithmName())
        .arg(m_signalLamps.size()).arg(m_model->decisionsByLink().size())
        .arg(m_demand.preloadTarget()).arg(m_requestedVehicleSeed)
        .arg(m_requestedDestinationSeed));
}

void MySimulator::afterStart()
{
    m_running = true;
    m_metrics.start(algorithmName(),
                    m_requestedVehicleSeed,
                    m_requestedDestinationSeed,
                    m_comparisonActive ? m_comparisonResultRound : 0,
                    m_comparisonActive ? m_comparisonBatchId : QString(),
                    m_comparisonActive ? m_comparisonRoundIndex : 0,
                    m_comparisonActive ? m_comparisonRoundsTotal : 0);
    emit eventGenerated(QStringLiteral("仿真已开始：信号、RSU聚合、车辆动作与路由均在C++内存中执行"));
}

void MySimulator::afterPause()
{
    emit eventGenerated(QStringLiteral("仿真已暂停"));
}

void MySimulator::afterStop()
{
    m_running = false;
    if (runtimeSmokeTestRequested()) {
        if (!m_runtimeSmokeCompleted)
            finishRuntimeSmokeTest(false, QStringLiteral("simulation stopped before smoke criteria"));
        QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
        return;
    }
    QString error;
    const QString result = m_metrics.finish(&m_routing, &error);
    if (!result.isEmpty()) emit eventGenerated(QStringLiteral("本轮结果已写入：%1").arg(result));
    else if (!error.isEmpty()) emit eventGenerated(QStringLiteral("结果写入失败：%1").arg(error));
    if (m_comparisonPendingStart) {
        m_comparisonPendingStart = false;
        beginComparisonBatch();
        return;
    }
    if (m_comparisonActive) {
        if (result.isEmpty()) {
            finishComparisonBatch(error.isEmpty()
                ? QStringLiteral("自动对照已终止：本次子实验没有可保存的数据")
                : QStringLiteral("自动对照已终止：结果保存失败：%1").arg(error));
            return;
        }
        if (m_comparisonCancelRequested) {
            finishComparisonBatch(QStringLiteral("自动对照已停止；已完成的子实验数据仍保留"));
            return;
        }
        if (m_comparisonAlgorithmIndex == 0) {
            m_comparisonAlgorithmIndex = 1;
            prepareComparisonRun();
            scheduleComparisonStart();
            return;
        }

        QString manifestError;
        if (!writeComparisonRoundManifest(&manifestError))
            emit eventGenerated(QStringLiteral("对照轮清单写入失败：%1").arg(manifestError));
        emit eventGenerated(QStringLiteral(
            "第%1/%2轮对照完成：round_%3，种子=%4/%5")
            .arg(m_comparisonRoundIndex).arg(m_comparisonRoundsTotal)
            .arg(m_comparisonResultRound, 3, 10, QChar('0'))
            .arg(m_comparisonVehicleSeed).arg(m_comparisonDestinationSeed));
        if (m_comparisonRoundIndex >= m_comparisonRoundsTotal) {
            finishComparisonBatch(QStringLiteral("自动对照完成：共%1轮、%2次子实验，数据已保存")
                .arg(m_comparisonRoundsTotal).arg(m_comparisonRoundsTotal * 2));
            return;
        }
        ++m_comparisonRoundIndex;
        m_comparisonAlgorithmIndex = 0;
        prepareComparisonRun();
        scheduleComparisonStart();
        return;
    }
    if (m_restartAfterStop) {
        m_restartAfterStop = false;
        QTimer::singleShot(350, this, [this]() {
            gpTessInterface->simuInterface()->startSimu();
        });
    }
}

void MySimulator::publishComparisonState(const QString& status)
{
    emit comparisonStateChanged(
        m_comparisonActive || m_comparisonPendingStart,
        m_comparisonRoundIndex,
        m_comparisonRoundsTotal,
        m_comparisonAlgorithmIndex == 0
            ? QStringLiteral("IMPROVED_MAX_PRESSURE")
            : QStringLiteral("FIXED_TIMING"),
        m_comparisonVehicleSeed,
        m_comparisonDestinationSeed,
        m_comparisonResultRound,
        status);
}

void MySimulator::beginComparisonBatch()
{
    m_comparisonActive = true;
    m_comparisonCancelRequested = false;
    m_comparisonRoundIndex = 1;
    m_comparisonAlgorithmIndex = 0;
    m_comparisonBatchId = QStringLiteral("batch_%1")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz")));
    prepareComparisonRun();
    scheduleComparisonStart();
}

void MySimulator::prepareComparisonRun()
{
    if (!m_comparisonActive || m_comparisonCancelRequested) return;
    if (m_comparisonAlgorithmIndex == 0) {
        m_comparisonResultRound = m_metrics.nextComparisonRound();
        const int previousVehicleSeed = m_comparisonVehicleSeed;
        const int previousDestinationSeed = m_comparisonDestinationSeed;
        do {
            m_comparisonVehicleSeed = static_cast<int>(
                QRandomGenerator::global()->generate() & 0x7fffffffu);
            m_comparisonDestinationSeed = static_cast<int>(
                QRandomGenerator::global()->generate() & 0x7fffffffu);
        } while (m_comparisonVehicleSeed == previousVehicleSeed
                 && m_comparisonDestinationSeed == previousDestinationSeed);
    }
    m_requestedBranch = m_comparisonAlgorithmIndex == 0
        ? ControlBranch::ImprovedMaxPressure : ControlBranch::FixedTiming;
    m_requestedVehicleSeed = m_comparisonVehicleSeed;
    m_requestedDestinationSeed = m_comparisonDestinationSeed;
    const QString status = m_comparisonAlgorithmIndex == 0
        ? QStringLiteral("准备运行") : QStringLiteral("切换基准算法");
    publishComparisonState(status);
    emit eventGenerated(QStringLiteral(
        "自动对照：第%1/%2轮，%3，round_%4，种子=%5/%6")
        .arg(m_comparisonRoundIndex).arg(m_comparisonRoundsTotal)
        .arg(m_comparisonAlgorithmIndex == 0
                 ? QStringLiteral("IMPROVED_MAX_PRESSURE")
                 : QStringLiteral("FIXED_TIMING"))
        .arg(m_comparisonResultRound, 3, 10, QChar('0'))
        .arg(m_comparisonVehicleSeed).arg(m_comparisonDestinationSeed));
}

void MySimulator::scheduleComparisonStart()
{
    QTimer::singleShot(350, this, [this]() {
        if (!m_comparisonActive || m_comparisonCancelRequested) return;
        SimuInterface* simulation = gpTessInterface->simuInterface();
        if (!simulation->isRunning()) simulation->startSimu();
    });
}

void MySimulator::finishComparisonBatch(const QString& status)
{
    const int completedRounds = m_comparisonAlgorithmIndex == 1
        ? m_comparisonRoundIndex : qMax(0, m_comparisonRoundIndex - 1);
    m_comparisonActive = false;
    m_comparisonPendingStart = false;
    m_comparisonCancelRequested = false;
    m_requestedBranch = m_manualBranchBeforeComparison;
    m_requestedVehicleSeed = m_manualVehicleSeedBeforeComparison;
    m_requestedDestinationSeed = m_manualDestinationSeedBeforeComparison;
    if (!m_running) m_signals.setBranch(m_requestedBranch);
    emit comparisonStateChanged(false, completedRounds, m_comparisonRoundsTotal,
                                QString(), -1, -1, 0, status);
    emit eventGenerated(status);
}

bool MySimulator::writeComparisonRoundManifest(QString* error) const
{
    if (!m_model || m_comparisonResultRound <= 0) return false;
    const QString roundName = QStringLiteral("round_%1")
        .arg(m_comparisonResultRound, 3, 10, QChar('0'));
    const QString directory = QDir(m_model->root()).filePath(
        QStringLiteral("result/%1").arg(roundName));
    if (!QDir().mkpath(directory)) {
        if (error) *error = directory;
        return false;
    }
    QJsonObject document;
    document.insert(QStringLiteral("schemaVersion"),
                    QStringLiteral("paired-comparison-round-v1"));
    document.insert(QStringLiteral("status"), QStringLiteral("COMPLETED"));
    document.insert(QStringLiteral("roundId"), roundName);
    document.insert(QStringLiteral("comparisonBatchId"), m_comparisonBatchId);
    document.insert(QStringLiteral("comparisonRoundIndex"), m_comparisonRoundIndex);
    document.insert(QStringLiteral("comparisonRoundCount"), m_comparisonRoundsTotal);
    document.insert(QStringLiteral("vehicleGenerationRandomSeed"),
                    m_comparisonVehicleSeed);
    document.insert(QStringLiteral("destinationChoiceRandomSeed"),
                    m_comparisonDestinationSeed);
    document.insert(QStringLiteral("durationSec"), m_durationSec);
    document.insert(QStringLiteral("preloadVehicleCount"), m_requestedPreloadCount);
    document.insert(QStringLiteral("outerRatePerEntryVehPerHour"), m_demand.outerRate());
    document.insert(QStringLiteral("residentialRatePerEntryVehPerHour"),
                    m_demand.residentialRate());
    document.insert(QStringLiteral("completedAt"),
                    QDateTime::currentDateTime().toString(Qt::ISODate));
    QJsonArray algorithms;
    algorithms.append(QStringLiteral("IMPROVED_MAX_PRESSURE"));
    algorithms.append(QStringLiteral("FIXED_TIMING"));
    document.insert(QStringLiteral("algorithms"), algorithms);
    QFile file(QDir(directory).filePath(QStringLiteral("comparison_manifest.json")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(document).toJson(QJsonDocument::Indented));
    return true;
}

void MySimulator::bindSignalLamps()
{
    m_signalLamps.clear();
    m_signalLampsByLane.clear();
    m_signalLampById.clear();
    int unmapped = 0;
    for (ISignalLamp* lamp : gpTessInterface->netInterface()->signalLamps()) {
        if (!lamp || !lamp->laneObject() || !lamp->laneObject()->isLane()) {
            ++unmapped;
            continue;
        }
        const qint64 laneId = lamp->laneObject()->id();
        if (!m_model->controlledIncomingStagesByLane().contains(laneId)) {
            ++unmapped;
            continue;
        }
        m_signalLamps.push_back(qMakePair(lamp, laneId));
        m_signalLampsByLane[laneId].push_back(lamp);
        m_signalLampById.insert(lamp->id(), lamp);
    }
    emit eventGenerated(QStringLiteral("信号绑定验收：已绑定%1个灯头，未纳管%2个")
        .arg(m_signalLamps.size()).arg(unmapped));
}

void MySimulator::writeSignalFrame()
{
    // Single writer: all physical lamps receive the same authoritative C++
    // lane-atomic state every 100 ms. No phase API and no vehicle guard can
    // independently invent another color.
    for (const auto& entry : m_signalLamps) {
        const qint64 lampId = entry.first->id();
        const QString color = m_signals.colorForLane(entry.second);
        if (m_expectedSignalColorByLampId.value(lampId) != color)
            m_signalColorStableSinceSec.insert(lampId, m_lastStepSec);
        m_expectedSignalColorByLampId.insert(lampId, color);
        entry.first->step()->setLampColor(color);
    }
}

QString MySimulator::actualSignalColorForLane(qint64 laneId) const
{
    // A physical approach lane can own several movement lamps.  The project
    // treats that lane atomically: any actual green releases it, otherwise
    // yellow has priority over red.  Falling back to the planned frame is
    // allowed only when the SDK has no readable lamp for this lane.
    QString result;
    for (ISignalLamp* lamp : m_signalLampsByLane.value(laneId)) {
        if (!lamp) continue;
        const QString color = lamp->color().trimmed().toUpper();
        if (color == "G") return "G";
        if (color == "Y") result = "Y";
        else if (color == "R" && result.isEmpty()) result = "R";
    }
    return result.isEmpty() ? m_signals.colorForLane(laneId) : result;
}

void MySimulator::verifySignalReadback(double simulationTimeSec)
{
    const double period = qMax(1.0, m_model->scenario()
        .value("simulationPerformance").toObject()
        .value("visibleEfficiency").toObject()
        .value("signalReadbackPeriodSec").toDouble(5.0));
    if (simulationTimeSec < m_lastSignalReadbackSec + period) return;
    m_lastSignalReadbackSec = simulationTimeSec;
    m_signalReadbackTimeSec = simulationTimeSec;
    QSet<int> abnormal;
    for (const auto& entry : m_signalLamps) {
        ISignalLamp* lamp = entry.first;
        const qint64 lampId = lamp->id();
        const QString expected = m_expectedSignalColorByLampId.value(lampId);
        if (expected.isEmpty()) continue;
        const QString actual = lamp->color().trimmed().toUpper();
        const bool readable = actual == "R" || actual == "Y" || actual == "G";
        const bool stable = simulationTimeSec
            - m_signalColorStableSinceSec.value(lampId, simulationTimeSec) >= 1.0;
        if (readable && (!stable || actual == expected)) continue;
        for (const StageRef& ref : m_model->controlledIncomingStagesByLane().value(entry.second))
            if (ref.intersection >= 0) abnormal.insert(ref.intersection);
    }
    m_signalReadbackAbnormalIntersections = abnormal.size();
    if (!abnormal.isEmpty() && simulationTimeSec >= m_lastSignalWarningSec + 10.0) {
        m_lastSignalWarningSec = simulationTimeSec;
        QStringList ids;
        for (int index : abnormal)
            ids.push_back(m_model->intersections().value(index).id);
        ids.sort();
        emit eventGenerated(QStringLiteral(
            "灯色实际回读异常：%1。C++计划与实体灯色不一致，下一批将由唯一写入器重新下发。")
            .arg(ids.mid(0, 12).join(", ")));
    }
}

void MySimulator::executeVehicleSafety(const QList<IVehicle*>& vehicles)
{
    QSet<qint64> liveVehicleIds;
    QHash<qint64, int> activeLaneChangesByLink;
    m_connectorStallDetails.clear();
    m_permissiveRightYieldVehicleIds.clear();
    m_wrongExitBlockedVehicleIds.clear();
    const QJsonObject& scenario = m_model->scenario();
    const QJsonObject signalConfig = scenario.value("signals").toObject();
    const QJsonObject cavConfig = scenario.value("cav").toObject();
    const double stopGuardM = signalConfig
        .value("vehicleRedStopGuardM").toDouble(60.0);
    const double hardStopM = signalConfig
        .value("vehicleHardStopLineM").toDouble(3.0);
    const double releaseGuardM = signalConfig
        .value("vehicleGreenReleaseGuardM").toDouble(5.0);
    const double releaseSpeedMps = signalConfig
        .value("vehicleGreenReleaseSpeedMps").toDouble(0.8);
    const int maximumGreenReleaseImpulseAttempts = qMax(0, signalConfig
        .value("vehicleGreenReleaseMaximumImpulseAttempts").toInt(1));
    const double releaseMaxOccupancy = signalConfig
        .value("greenReleaseMaxDownstreamOccupancy").toDouble(0.82);
    const double releaseFrontClearanceM = signalConfig
        .value("vehicleGreenReleaseFrontClearanceM").toDouble(6.0);
    const bool permissiveRightTurn = signalConfig
        .value("permissiveRightTurn").toBool(true);
    const double permissiveRightQueueThreshold = signalConfig
        .value("permissiveRightProtectedQueueThresholdVeh").toDouble(0.5);
    const QJsonObject laneBalancingConfig =
        cavConfig.value("laneBalancing").toObject();
    const double laneFreezeDistanceM = laneBalancingConfig
        .value("freezeDistanceBeforeStopLineM").toDouble(60.0);
    const double laneRecoveryDistanceM = qMax(laneFreezeDistanceM,
        laneBalancingConfig.value("laneChangeInitiationCutoffBeforeStopLineM")
            .toDouble(140.0));
    const double connectorSelectionLookaheadM = qMax(laneRecoveryDistanceM,
        laneBalancingConfig.value("connectorSelectionLookaheadM")
            .toDouble(320.0));
    const double queueLaneLockDistanceM = qBound(laneFreezeDistanceM,
        laneBalancingConfig.value("queueLaneLockDistanceM").toDouble(90.0),
        laneRecoveryDistanceM);
    const double queueAnticipationFrontDistanceM = qMax(0.0,
        laneBalancingConfig.value("queueAnticipationFrontDistanceM")
            .toDouble(30.0));
    const double queueSpeedThresholdMps = qMax(0.0,
        laneBalancingConfig.value("queueSpeedThresholdMps").toDouble(2.0));
    const double connectorReselectionIntervalSec = qMax(1.0,
        laneBalancingConfig.value("connectorReselectionIntervalSec")
            .toDouble(5.0));
    const double connectorReselectionMinimumScoreGain = qMax(0.0,
        laneBalancingConfig.value("connectorReselectionMinimumScoreGain")
            .toDouble(6.0));
    // Never change the receiving lane after the approach has entered the
    // no-new-lane-change zone. A connector change there creates a new curved
    // trace at the stop line and can leave the vehicle diagonally across two
    // lanes. Spillback escape therefore has to happen upstream as well.
    const double connectorEmergencyReselectionMinDistanceM = qMax(
        laneRecoveryDistanceM,
        laneBalancingConfig.value("connectorEmergencyReselectionMinDistanceM")
            .toDouble(12.0));
    const double connectorEmergencyBlockOccupancy = qBound(0.5,
        laneBalancingConfig.value("connectorEmergencyBlockOccupancy")
            .toDouble(0.85), 1.0);
    const double connectorEmergencyReleaseOccupancy = qBound(0.0,
        laneBalancingConfig.value("connectorEmergencyReleaseOccupancy")
            .toDouble(0.65), connectorEmergencyBlockOccupancy);
    const int maximumConnectorReselectionsPerLink = qMax(0,
        laneBalancingConfig.value("maximumConnectorReselectionsPerLink")
            .toInt(1));
    const double minimumMandatoryLaneChangeSpeedMps = qMax(0.0,
        laneBalancingConfig.value("minimumMandatoryLaneChangeSpeedMps")
            .toDouble(4.0));
    const double maximumMandatoryLaneChangeOccupancy = qBound(0.0,
        laneBalancingConfig.value("maximumMandatoryLaneChangeOccupancy")
            .toDouble(0.55), 1.0);
    const double maximumMandatoryTargetLaneOccupancy = qBound(0.0,
        laneBalancingConfig.value("maximumMandatoryTargetLaneOccupancy")
            .toDouble(0.55), 1.0);
    const double minimumMandatoryFrontGapM = qMax(0.0,
        laneBalancingConfig.value("minimumMandatoryFrontGapM")
            .toDouble(14.0));
    const double minimumMandatoryRearGapM = qMax(0.0,
        laneBalancingConfig.value("minimumMandatoryRearGapM")
            .toDouble(12.0));
    const double rearClosingTimeBufferSec = qMax(0.0,
        laneBalancingConfig.value("rearClosingTimeBufferSec")
            .toDouble(1.0));
    const double minimumLaneChangeStartIntervalSecPerLink = qMax(0.0,
        laneBalancingConfig.value("minimumLaneChangeStartIntervalSecPerLink")
            .toDouble(1.5));
    const int maximumConcurrentLaneChangesPerLink = qMax(1,
        laneBalancingConfig.value("maximumConcurrentLaneChangesPerLink")
            .toInt(1));
    const double approachLaneChangeSafeDistanceM = qMax(0.0,
        laneBalancingConfig.value("approachLaneChangeSafeDistanceM")
            .toDouble(12.0));
    for (IVehicle* vehicle : vehicles) {
        if (!vehicle || !vehicle->roadIsLink() || !vehicle->vehicleDriving())
            continue;
        if (!vehicle->vehicleDriving()->changingTrace(Metric).isEmpty())
            ++activeLaneChangesByLink[vehicle->roadId()];
    }
    const QJsonObject deadlockConfig = signalConfig
        .value("connectorDeadlockRecovery").toObject();
    const bool connectorDeadlockRecoveryEnabled =
        deadlockConfig.value("enabled").toBool(true);
    const double connectorDeadlockSec = qMax(3.0,
        deadlockConfig.value("stoppedThresholdSec").toDouble(8.0));
    const double protectedGreenConnectorStoppedSec = qBound(0.2,
        deadlockConfig.value("protectedGreenStoppedThresholdSec")
            .toDouble(0.8), 2.0);
    const double connectorDeadlockFrontClearanceM = qMax(6.0,
        deadlockConfig.value("frontClearanceM").toDouble(20.0));
    const double connectorDeadlockMaxOccupancy = qBound(0.0,
        deadlockConfig.value("maximumDownstreamOccupancy").toDouble(0.35), 1.0);
    const double connectorDeadlockCreepSpeedMps = qBound(0.2,
        deadlockConfig.value("creepSpeedMps").toDouble(0.8), 1.5);
    const double protectedGreenConnectorClearSpeedMps = qBound(0.8,
        deadlockConfig.value("protectedGreenClearSpeedMps")
            .toDouble(2.0), 3.0);
    const double connectorDeadlockRetryIntervalSec = qMax(0.5,
        deadlockConfig.value("retryIntervalSec").toDouble(1.5));
    const double connectorDeadlockIntersectionCooldownSec = qMax(0.2,
        deadlockConfig.value("perIntersectionReleaseCooldownSec")
            .toDouble(0.8));
    const int connectorDeadlockMaximumImpulseAttempts = qBound(1,
        deadlockConfig.value("maximumImpulseAttempts").toInt(3), 5);
    const auto& graphLinks = m_model->links();
    const auto& controlledIncomingStages =
        m_model->controlledIncomingStagesByLane();
    const auto& turnTypesByLane = m_model->turnTypeByLaneTarget();
    static const QHash<qint64, QString> emptyLaneTurns;
    for (IVehicle* vehicle : vehicles) {
        if (!vehicle) continue;
        liveVehicleIds.insert(vehicle->id());
        if (!vehicle->roadIsLink() || !vehicle->vehicleDriving()) {
            // Once the vehicle has entered a connector, the stop-line hold
            // that belonged to its previous incoming lane is no longer valid.
            m_signalHardStoppedVehicleIds.remove(vehicle->id());
            m_greenStoppedSinceSec.remove(vehicle->id());
            m_greenReleaseLastImpulseSec.remove(vehicle->id());
            m_greenReleaseImpulseAttempts.remove(vehicle->id());
            ILaneConnector* laneConnector = vehicle->laneConnector();
            if (!vehicle->roadIsLink() && vehicle->vehicleDriving()
                && laneConnector && laneConnector->toLane()
                && laneConnector->toLane()->link()) {
                // setDesirSpeed(0) is persistent across lane objects. A vehicle
                // released at the stop line can therefore carry an old red or
                // permissive-right yield command into the connector and stop
                // there forever. Restore only desired speed on every connector;
                // TESSNG's native following/conflict model still owns actual
                // speed and collision avoidance.
                const qint64 downstreamLinkId =
                    laneConnector->toLane()->link()->id();
                const GraphLink downstream = m_model->links().value(downstreamLinkId);
                const QJsonObject cavConfig =
                    m_model->scenario().value("cav").toObject();
                const double minimumAdvice = qMax(0.1,
                    cavConfig.value("minimumAdviceMps").toDouble(3.0));
                const double maximumAdvice = qMax(minimumAdvice,
                    cavConfig.value("maximumAdviceMps").toDouble(16.67));
                const double typeLimit = vehicle->vehicleTypeCode() == 14
                    ? 55.0 / 3.6 : 60.0 / 3.6;
                const double desired = qBound(minimumAdvice,
                    qMin(typeLimit, qMax(1.0, downstream.freeSpeedMps)),
                    maximumAdvice);
                vehicle->step()->setDesirSpeed(m2p(desired));
            }
            // A recovery series belongs to one continuous stopped episode.
            // Reset it as soon as native motion resumes so a later independent
            // blockage can be diagnosed and recovered without a permanent
            // per-vehicle one-shot lockout.
            if (!vehicle->roadIsLink() && vehicle->vehicleDriving()
                && vehicle->vehicleDriving()->zeroSpeedInterval() < 500) {
                m_connectorDeadlockRecoveryVehicleIds.remove(vehicle->id());
                m_connectorDeadlockLastImpulseSec.remove(vehicle->id());
                m_connectorDeadlockImpulseAttempts.remove(vehicle->id());
            }
            if (!vehicle->roadIsLink() && vehicle->vehicleDriving()
                && vehicle->vehicleDriving()->zeroSpeedInterval() >= 3000) {
                QVariantMap detail;
                detail.insert(QStringLiteral("zeroSpeedSec"),
                              vehicle->vehicleDriving()->zeroSpeedInterval() / 1000.0);
                detail.insert(QStringLiteral("speedMps"), vehicle->currSpeed(Metric));
                detail.insert(QStringLiteral("desiredSpeedMps"),
                              vehicle->vehicleDriving()->desirSpeed(Metric));
                detail.insert(QStringLiteral("distanceToConnectorEndM"),
                              vehicle->vehicleDriving()->distToEndpoint(
                                  true, true, Metric));
                detail.insert(QStringLiteral("vehicleTypeCode"), vehicle->vehicleTypeCode());
                qint64 fromLinkId = 0;
                qint64 toLinkId = 0;
                if (laneConnector) {
                    detail.insert(QStringLiteral("laneConnectorId"),
                                  laneConnector->id());
                    detail.insert(QStringLiteral("connectorId"),
                                  laneConnector->connector()
                                      ? laneConnector->connector()->id() : 0);
                    fromLinkId = laneConnector->fromLane()
                        && laneConnector->fromLane()->link()
                        ? laneConnector->fromLane()->link()->id() : 0;
                    toLinkId = laneConnector->toLane()
                        && laneConnector->toLane()->link()
                        ? laneConnector->toLane()->link()->id() : 0;
                    detail.insert(QStringLiteral("fromLinkId"), fromLinkId);
                    detail.insert(QStringLiteral("toLinkId"), toLinkId);
                    detail.insert(QStringLiteral("toLaneId"),
                                  laneConnector->toLane()
                                      ? laneConnector->toLane()->id() : 0);
                    if (laneConnector->toLane()) {
                        const LaneTraffic downstreamLane = m_traffic.lanes()
                            .value(laneConnector->toLane()->id());
                        detail.insert(QStringLiteral("downstreamLaneVehicles"),
                                      downstreamLane.vehicleCount);
                        detail.insert(QStringLiteral("downstreamLaneQueue"),
                                      downstreamLane.queueCount);
                        detail.insert(QStringLiteral("downstreamLaneOccupancy"),
                                      downstreamLane.occupancy);
                    }
                }
                IVehicle* front = vehicle->vehicleFront();
                detail.insert(QStringLiteral("frontVehicleId"),
                              front ? front->id() : 0);
                const double frontDistanceM = vehicle->vehiDistFront(Metric);
                detail.insert(QStringLiteral("frontDistanceM"), frontDistanceM);
                // vehicleFront() can be a conflict vehicle selected by
                // TESSNG's connector-yield model rather than a physical leader
                // on this vehicle's path. Treating every such vehicle as a
                // same-lane leader prevents a protected straight stream from
                // breaking a mutual-yield lock.
                bool frontOnSamePath = false;
                bool frontQueuedOnRedApproach = false;
                bool frontInsideDifferentConnector = false;
                QString frontRelationship = QStringLiteral("NONE");
                if (front && laneConnector) {
                    if (!front->roadIsLink() && front->laneConnector()) {
                        ILaneConnector* frontConnector = front->laneConnector();
                        if (frontConnector->id() == laneConnector->id()) {
                            frontOnSamePath = true;
                            frontRelationship = QStringLiteral("SAME_CONNECTOR");
                        } else if (frontConnector->toLane()
                                   && laneConnector->toLane()
                                   && frontConnector->toLane()->id()
                                       == laneConnector->toLane()->id()) {
                            frontOnSamePath = true;
                            frontRelationship = QStringLiteral("SAME_RECEIVING_LANE");
                        } else {
                            frontInsideDifferentConnector = true;
                            frontRelationship = QStringLiteral("CROSS_CONFLICT");
                        }
                    } else if (front->roadIsLink() && laneConnector->toLane()
                               && front->laneId() == laneConnector->toLane()->id()) {
                        frontOnSamePath = true;
                        frontRelationship = QStringLiteral("DOWNSTREAM_LEADER");
                    } else {
                        frontRelationship = QStringLiteral("CROSS_CONFLICT");
                        frontQueuedOnRedApproach = front->roadIsLink()
                            && actualSignalColorForLane(front->laneId())
                                == QStringLiteral("R")
                            && front->currSpeed(Metric) <= 0.5;
                    }
                }
                detail.insert(QStringLiteral("frontRelationship"),
                              frontRelationship);
                QStringList intersectionIds;
                const QVector<StageRef> refs = m_model->movementStagesByLink()
                    .value(fromLinkId).value(toLinkId);
                bool permissiveOpposingMovement = false;
                bool exclusiveProtectedStageIsGreen = false;
                bool activeSignalStageIsGreen = false;
                bool permanentGreenMovement = false;
                const bool sourceLaneActualGreen = laneConnector
                    && laneConnector->fromLane()
                    && actualSignalColorForLane(
                        laneConnector->fromLane()->id()) == QStringLiteral("G");
                for (const StageRef& ref : refs) {
                    if (ref.intersection < 0
                        || ref.intersection >= m_model->intersections().size()
                        || ref.intersection >= m_signals.states().size())
                        continue;
                    const IntersectionDefinition& intersection =
                        m_model->intersections().at(ref.intersection);
                    if (ref.stage < 0 || ref.stage >= intersection.stages.size())
                        continue;
                    const StageDefinition& stage =
                        intersection.stages.at(ref.stage);
                    const QString id = m_model->intersections()
                        .value(ref.intersection).id;
                    if (!id.isEmpty() && !intersectionIds.contains(id))
                        intersectionIds.push_back(id);
                    if (stage.permissiveOpposing)
                        permissiveOpposingMovement = true;
                    const IntersectionSignalState& signalState =
                        m_signals.states().at(ref.intersection);
                    if (sourceLaneActualGreen && intersection.permanentGreen) {
                        permanentGreenMovement = true;
                        exclusiveProtectedStageIsGreen = true;
                    } else if (sourceLaneActualGreen
                        && signalState.transition == SignalTransition::Green
                        && signalState.activeStage == ref.stage) {
                        activeSignalStageIsGreen = true;
                        if (!stage.permissiveOpposing
                            && stage.concurrentApproaches <= 1)
                            exclusiveProtectedStageIsGreen = true;
                    }
                }
                QString connectorTurnType;
                if (laneConnector && laneConnector->fromLane())
                    connectorTurnType = m_model->turnTypeByLaneTarget()
                        .value(laneConnector->fromLane()->id())
                        .value(toLinkId).trimmed().toLower();
                // In a circular-green opposing pair, the straight stream has
                // priority and the opposing left turn yields. Allow recovery
                // only for that priority stream; a permissive left/right turn
                // remains fully owned by TESSNG's native conflict model.
                const bool hasProtectedPriority = !permissiveOpposingMovement
                    || connectorTurnType == QStringLiteral("straight");
                const bool protectedPriorityGreen = sourceLaneActualGreen
                    && (permanentGreenMovement
                        || (activeSignalStageIsGreen && hasProtectedPriority));
                const bool crossConflictFrontStopped =
                    frontInsideDifferentConnector && front
                    && front->currSpeed(Metric) <= 0.5;
                // When two connector vehicles mutually report each other as
                // a stopped conflict leader, release only the lower stable
                // vehicle ID. The junction cooldown below serializes further
                // releases; same-path leaders are never bypassed.
                const bool winsCrossConflictTie = crossConflictFrontStopped
                    && vehicle->id() < front->id();
                detail.insert(QStringLiteral("turnType"), connectorTurnType);
                detail.insert(QStringLiteral("protectedPriorityRecovery"),
                              hasProtectedPriority);
                detail.insert(QStringLiteral("intersectionIds"),
                              intersectionIds.join(','));
                const double distanceToConnectorEndM =
                    vehicle->vehicleDriving()->distToEndpoint(true, true, Metric);
                const double downstreamOccupancy = detail.value(
                    QStringLiteral("downstreamLaneOccupancy"), 1.0).toDouble();
                // On a single-approach protected phase (J251 is one example),
                // a queued vehicle on the red opposing approach can still be
                // exposed by TESSNG as a conflict "front" object. It must not
                // hold a protected left turn inside the junction until the
                // opposing phase starts. Clear that connector promptly while
                // its source lamp is physically green and the real downstream
                // lane has space. Same-path leaders remain authoritative.
                const bool protectedGreenPathClear =
                    protectedPriorityGreen
                    && (!front || frontQueuedOnRedApproach
                        || winsCrossConflictTie
                        || (frontOnSamePath
                            && frontDistanceM
                                >= connectorDeadlockFrontClearanceM));
                const bool deadlockPathClear = connectorDeadlockRecoveryEnabled
                    && hasProtectedPriority
                    && !refs.isEmpty()
                    && distanceToConnectorEndM > 0.5
                    && (!frontInsideDifferentConnector
                        || winsCrossConflictTie)
                    && (!frontOnSamePath
                        || frontDistanceM >= connectorDeadlockFrontClearanceM)
                    && downstreamOccupancy <= connectorDeadlockMaxOccupancy;
                const double stoppedThresholdSec = protectedGreenPathClear
                    ? protectedGreenConnectorStoppedSec : connectorDeadlockSec;
                const bool deadlockCandidate = deadlockPathClear
                    && vehicle->vehicleDriving()->zeroSpeedInterval()
                        >= qRound64(stoppedThresholdSec * 1000.0);
                const int recoveryAttempts =
                    m_connectorDeadlockImpulseAttempts.value(vehicle->id(), 0);
                const bool recoveryRetryReady =
                    !m_connectorDeadlockRecoveryVehicleIds.contains(vehicle->id())
                    || m_lastStepSec - m_connectorDeadlockLastImpulseSec
                        .value(vehicle->id(), -1e9)
                        >= connectorDeadlockRetryIntervalSec;
                const QString recoveryJunctionKey = !intersectionIds.isEmpty()
                    ? intersectionIds.first()
                    : QStringLiteral("CONNECTOR_%1").arg(
                        laneConnector && laneConnector->connector()
                            ? laneConnector->connector()->id() : 0);
                const bool intersectionReleaseReady =
                    m_lastStepSec - m_connectorDeadlockLastIntersectionReleaseSec
                        .value(recoveryJunctionKey, -1e9)
                        >= connectorDeadlockIntersectionCooldownSec;
                const bool deadlockRecoveryStarted = deadlockCandidate
                    && recoveryAttempts < connectorDeadlockMaximumImpulseAttempts
                    && recoveryRetryReady && intersectionReleaseReady;
                if (deadlockRecoveryStarted) {
                    m_connectorDeadlockRecoveryVehicleIds.insert(vehicle->id());
                    m_connectorDeadlockLastImpulseSec.insert(
                        vehicle->id(), m_lastStepSec);
                    m_connectorDeadlockImpulseAttempts.insert(
                        vehicle->id(), recoveryAttempts + 1);
                    m_connectorDeadlockLastIntersectionReleaseSec.insert(
                        recoveryJunctionKey, m_lastStepSec);
                    ++m_connectorDeadlockReleaseImpulseCount;
                }
                if (!deadlockPathClear) {
                    m_connectorDeadlockRecoveryVehicleIds.remove(vehicle->id());
                    m_connectorDeadlockLastImpulseSec.remove(vehicle->id());
                    m_connectorDeadlockImpulseAttempts.remove(vehicle->id());
                }
                if (deadlockRecoveryStarted) {
                    // The vehicle is already inside a signalized connector,
                    // with no same-path leader and free downstream space. Use
                    // a few widely spaced impulses for a continuous stop, not
                    // a 10 Hz speed override; this clears a persistent native
                    // mutual-yield lock without visible red/blue oscillation.
                    vehicle->step()->setSpeed(m2p(
                        protectedGreenPathClear
                            ? protectedGreenConnectorClearSpeedMps
                            : connectorDeadlockCreepSpeedMps));
                    detail.insert(QStringLiteral("deadlockRecoveryApplied"), true);
                }
                detail.insert(QStringLiteral("sourceLaneActualGreen"),
                              sourceLaneActualGreen);
                detail.insert(QStringLiteral("exclusiveProtectedStageIsGreen"),
                              exclusiveProtectedStageIsGreen);
                detail.insert(QStringLiteral("protectedGreenPathClear"),
                              protectedGreenPathClear);
                detail.insert(QStringLiteral("protectedPriorityGreen"),
                              protectedPriorityGreen);
                detail.insert(QStringLiteral("crossConflictFrontStopped"),
                              crossConflictFrontStopped);
                detail.insert(QStringLiteral("winsCrossConflictTie"),
                              winsCrossConflictTie);
                detail.insert(QStringLiteral("frontQueuedOnRedApproach"),
                              frontQueuedOnRedApproach);
                detail.insert(QStringLiteral("frontInsideDifferentConnector"),
                              frontInsideDifferentConnector);
                detail.insert(QStringLiteral("recoveryStoppedThresholdSec"),
                              stoppedThresholdSec);
                detail.insert(QStringLiteral("deadlockRecoveryAttempts"),
                              m_connectorDeadlockImpulseAttempts
                                  .value(vehicle->id(), 0));
                m_connectorStallDetails.insert(vehicle->id(), detail);
            }
            continue;
        }
        m_connectorDeadlockRecoveryVehicleIds.remove(vehicle->id());
        m_connectorDeadlockLastImpulseSec.remove(vehicle->id());
        m_connectorDeadlockImpulseAttempts.remove(vehicle->id());
        m_lastVehicleLinkIds.insert(vehicle->id(), vehicle->roadId());
        const qint64 currentLinkId = vehicle->roadId();
        const qint64 decisionOwner = m_model->decisionOwnerByCorridorLink()
            .value(currentLinkId, 0);
        if (decisionOwner > 0) {
            const VehicleOdState registeredOd=
                m_routing.vehicleOdState(vehicle->id());
            if (registeredOd.destinationExitLinkId>0
                && !m_routing.ensureVehicleRoute(vehicle, currentLinkId)) {
                const VehicleOdState od=m_routing.vehicleOdState(vehicle->id());
                m_routeMaintenanceFailures.insert(QStringLiteral("%1:%2:%3:%4:%5")
                    .arg(vehicle->id()).arg(decisionOwner).arg(currentLinkId)
                    .arg(od.destinationExitLinkId).arg(od.originEntryLinkId));
                vehicle->setJsonProperty(
                    QStringLiteral("routeMaintenanceFailed"), true);
            }
        }
        const qint64 laneId = vehicle->laneId();
        const bool signalLane = controlledIncomingStages.contains(laneId);
        const bool intersectionApproach = signalLane
            || m_model->incomingStagesByLink().contains(currentLinkId);
        const double distanceToEnd = vehicle->vehicleDriving()->distToEndpoint(true, true, Metric);
        IVehicle::Step* command = vehicle->step();
        if (m_lockedVehicleConnectorFromLinkIds.value(vehicle->id(), 0)
            != vehicle->roadId()) {
            m_lockedVehicleConnectorIds.remove(vehicle->id());
            m_lockedVehicleConnectorFromLinkIds.remove(vehicle->id());
            m_lockedVehicleConnectorSelectedAtSec.remove(vehicle->id());
            m_lockedVehicleConnectorReselections.remove(vehicle->id());
        }
        const bool laneChangeInProgress =
            !vehicle->vehicleDriving()->changingTrace(Metric).isEmpty();
        IVehicle* approachFrontVehicle = vehicle->vehicleFront();
        const double approachFrontDistanceM = vehicle->vehiDistFront(Metric);
        const bool queuedApproachLaneLock = intersectionApproach
            && laneBalancingConfig.value(
                "queueLaneChangeProtectionEnabled").toBool(true)
            && distanceToEnd >= -1.0
            && distanceToEnd <= queueLaneLockDistanceM
            && (vehicle->currSpeed(Metric) <= queueSpeedThresholdMps
                || (approachFrontVehicle
                    && approachFrontDistanceM >= 0.0
                    && approachFrontDistanceM <= queueAnticipationFrontDistanceM
                    && approachFrontVehicle->currSpeed(Metric)
                        <= queueSpeedThresholdMps));
        const bool finalStopLineLaneLock =
            intersectionApproach && distanceToEnd >= -1.0
            && distanceToEnd <= queueLaneLockDistanceM;
        const bool laneChangeLocked =
            m_laneBalancer.isLaneChangeLocked(vehicle->roadId())
            || finalStopLineLaneLock;
        const bool laneChangeInitiationLocked = laneChangeLocked
            || (intersectionApproach && distanceToEnd <= laneRecoveryDistanceM);
        if (laneChangeLocked) {
            m_laneBalancer.cancelVehicleCommand(vehicle->id());
            // calcLimitedLaneNumber and beforeTo*Freely are the only native
            // writers that prevent a *new* manoeuvre here. Never dismiss or
            // overwrite a change whose trace already exists: interrupting it
            // in the final metres is what leaves a vehicle sideways and makes
            // TESSNG recreate the manoeuvre on the next calculation step.
        }
        const auto graphLinkIt = graphLinks.constFind(vehicle->roadId());
        const double graphFreeSpeedMps = graphLinkIt == graphLinks.cend()
            ? 1.0 : graphLinkIt.value().freeSpeedMps;
        const double typeLimit = vehicle->vehicleTypeCode() == 14 ? 55.0 / 3.6 : 60.0 / 3.6;
        const double minimumAdvice = qMax(0.1,
            cavConfig.value("minimumAdviceMps").toDouble(3.0));
        const double maximumAdvice = qMax(minimumAdvice,
            cavConfig.value("maximumAdviceMps").toDouble(16.67));
        const double freeFlow = qBound(minimumAdvice,
            qMin(typeLimit, qMax(1.0, graphFreeSpeedMps)), maximumAdvice);
        const auto laneTurnsIt = turnTypesByLane.constFind(laneId);
        const QHash<qint64, QString>& laneTurns =
            laneTurnsIt == turnTypesByLane.cend()
                ? emptyLaneTurns : laneTurnsIt.value();
        QVector<qint64> reachableTargets;
        reachableTargets.reserve(laneTurns.size());
        for (auto it = laneTurns.cbegin(); it != laneTurns.cend(); ++it)
            reachableTargets.push_back(it.key());
        if(reachableTargets.isEmpty()&&vehicle->lane()){
            for(ILaneConnector*connector:vehicle->lane()->toLaneConnectors()){
                if(!connector||!connector->toLane()
                    ||!connector->toLane()->link())continue;
                const qint64 candidate=connector->toLane()->link()->id();
                if(!reachableTargets.contains(candidate))
                    reachableTargets.push_back(candidate);
            }
        }
        qint64 targetLinkId = m_routing.movementTargetLink(
            vehicle->id(), vehicle->roadId());
        const VehicleOdState activeOd=m_routing.vehicleOdState(vehicle->id());
        QVector<qint64> odSafeTargets;
        bool laneOnlyOffersWrongExit=!reachableTargets.isEmpty();
        for(qint64 candidate:reachableTargets){
            const bool wrongExit=activeOd.destinationExitLinkId>0
                &&candidate!=activeOd.destinationExitLinkId
                &&(m_model->terminalIndexByExitLink().contains(candidate)
                    ||m_model->forbiddenExitLinkIds().contains(candidate));
            if(!wrongExit){
                odSafeTargets.push_back(candidate);
                laneOnlyOffersWrongExit=false;
            }
        }
        const LaneTraffic currentLaneTraffic = m_traffic.lanes().value(laneId);
        const bool congestedCurrentLane = currentLaneTraffic.vehicleCount > 0
            && (currentLaneTraffic.meanSpeedMps <= laneBalancingConfig
                    .value("congestedLaneRecoverySpeedMps").toDouble(4.0)
                || currentLaneTraffic.occupancy >= laneBalancingConfig
                    .value("congestedLaneRecoveryOccupancy").toDouble(0.55));
        // Preserve the OD-selected movement while there is still room to
        // change lanes. The old order recovered to the current lane before the
        // lane balancer could align the vehicle, causing network-wide late
        // reroutes. Recovery is now only the stop-line safety fallback.
        if ((m_laneBalancer.isLaneChangeLocked(vehicle->roadId())
             || distanceToEnd <= laneRecoveryDistanceM
             || queuedApproachLaneLock
             || (congestedCurrentLane
                 && distanceToEnd <= connectorSelectionLookaheadM))
            && !odSafeTargets.isEmpty()
            && !odSafeTargets.contains(targetLinkId)) {
            const qint64 staleTargetLinkId = targetLinkId;
            targetLinkId = m_routing.recoverReachableMovementTarget(
                vehicle->id(), vehicle->roadId(), odSafeTargets);
            vehicle->setJsonProperty(QStringLiteral("nextLinkId"), targetLinkId);
            if (targetLinkId > 0 && targetLinkId != staleTargetLinkId) {
                ++m_recoveredMovementCount;
                m_routing.synchronizeNativeRoute(
                    vehicle, vehicle->roadId(), targetLinkId);
            }
        }
        // If a vehicle reaches the no-new-lane-change zone in a lane whose
        // only physical movements are wrong terminal exits, its requested OD
        // is no longer physically recoverable at this junction. Do not keep
        // it forever on a green stop line. Select the least loaded real exit
        // connector as a fail-open evacuation path; the original fixed OD is
        // retained so the departure is still reported as abnormal, never as
        // a successful completion. Earlier congested-lane recovery prevents
        // this fallback whenever any destination-preserving movement exists.
        if (laneOnlyOffersWrongExit && activeOd.destinationExitLinkId > 0
            && distanceToEnd >= -1.0
            && distanceToEnd <= laneRecoveryDistanceM) {
            qint64 evacuationTarget = 0;
            double evacuationScore = std::numeric_limits<double>::infinity();
            if (vehicle->lane()) {
                for (ILaneConnector* connector
                     : vehicle->lane()->toLaneConnectors()) {
                    if (!connector || !connector->toLane()
                        || !connector->toLane()->link())
                        continue;
                    const qint64 candidate = connector->toLane()->link()->id();
                    if (!reachableTargets.contains(candidate)) continue;
                    const LaneTraffic receiving = m_traffic.lanes()
                        .value(connector->toLane()->id());
                    const double score = 5.0 * receiving.queueCount
                        + 2.0 * receiving.vehicleCount
                        + 10.0 * receiving.occupancy
                        + candidate * 1e-12;
                    if (score < evacuationScore) {
                        evacuationScore = score;
                        evacuationTarget = candidate;
                    }
                }
            }
            if (evacuationTarget <= 0) {
                for (qint64 candidate : reachableTargets)
                    if (evacuationTarget <= 0 || candidate < evacuationTarget)
                        evacuationTarget = candidate;
            }
            if (evacuationTarget > 0) {
                targetLinkId = evacuationTarget;
                vehicle->setJsonProperty(
                    QStringLiteral("nextLinkId"), targetLinkId);
                vehicle->setJsonProperty(
                    QStringLiteral("routingExecution"),
                    QStringLiteral("WRONG_EXIT_PHYSICAL_FAIL_OPEN"));
                vehicle->setJsonProperty(
                    QStringLiteral("wrongExitPhysicalFailOpen"), true);
            }
        }
        // Actively bind the executable connector instead of depending on the
        // late native callback alone. Among connectors serving the same fixed
        // OD movement, select the least occupied downstream lane. This keeps
        // vehicles distributed when the next link widens and prevents every
        // vehicle selecting the same lane before a lane drop.
        if (targetLinkId > 0 && distanceToEnd >= -1.0
            && distanceToEnd <= connectorSelectionLookaheadM
            && vehicle->lane() && !laneChangeInProgress) {
            QList<ILaneConnector*> targetConnectors;
            ILaneConnector* bestConnector = nullptr;
            ILaneConnector* leastLoadedConnector = nullptr;
            double bestConnectorScore = std::numeric_limits<double>::infinity();
            double lockedConnectorScore = std::numeric_limits<double>::infinity();
            double lockedConnectorOccupancy = 1.0;
            double leastLoadedConnectorOccupancy = 1.0;
            const qint64 lockedConnectorId =
                m_lockedVehicleConnectorIds.value(vehicle->id(), 0);
            for (ILaneConnector* connector : vehicle->lane()->toLaneConnectors()) {
                if (!connector || !connector->toLane()
                    || !connector->toLane()->link()
                    || connector->toLane()->link()->id() != targetLinkId)
                    continue;
                targetConnectors.push_back(connector);
                const LaneTraffic lane = m_traffic.lanes()
                    .value(connector->toLane()->id());
                const double score = 5.0 * lane.queueCount
                    + 2.0 * lane.vehicleCount + 10.0 * lane.occupancy
                    + connector->toLane()->id() * 1e-12;
                if (connector->id() == lockedConnectorId) {
                    bestConnector = connector;
                    lockedConnectorScore = score;
                    lockedConnectorOccupancy = lane.occupancy;
                }
                if (score < bestConnectorScore) {
                    bestConnectorScore = score;
                    leastLoadedConnector = connector;
                    leastLoadedConnectorOccupancy = lane.occupancy;
                }
            }
            bool connectorReselected = false;
            if (!bestConnector) {
                bestConnector = leastLoadedConnector;
            } else if (leastLoadedConnector
                       && leastLoadedConnector != bestConnector) {
                const bool reselectionBudgetAvailable =
                    m_lockedVehicleConnectorReselections
                        .value(vehicle->id(), 0)
                        < maximumConnectorReselectionsPerLink;
                const bool normalUpstreamRefresh =
                    reselectionBudgetAvailable
                    &&
                    distanceToEnd > laneRecoveryDistanceM
                    && m_lastStepSec - m_lockedVehicleConnectorSelectedAtSec
                        .value(vehicle->id(), -1e9)
                        >= connectorReselectionIntervalSec
                    && bestConnectorScore + connectorReselectionMinimumScoreGain
                        < lockedConnectorScore;
                const bool oneEmergencyEscape =
                    reselectionBudgetAvailable
                    &&
                    distanceToEnd > connectorEmergencyReselectionMinDistanceM
                    && lockedConnectorOccupancy
                        >= connectorEmergencyBlockOccupancy
                    && leastLoadedConnectorOccupancy
                        <= connectorEmergencyReleaseOccupancy;
                if (normalUpstreamRefresh || oneEmergencyEscape) {
                    bestConnector = leastLoadedConnector;
                    connectorReselected = true;
                }
            }
            if (!targetConnectors.isEmpty() && bestConnector) {
                const bool connectorSelectionChanged = lockedConnectorId > 0
                    && bestConnector->id() != lockedConnectorId;
                QList<ILaneConnector*> committedConnectors;
                committedConnectors.push_back(bestConnector);
                command->setCandidateLaneConnectors(committedConnectors);
                command->setNextLaneConnector(bestConnector);
                m_lockedVehicleConnectorIds.insert(
                    vehicle->id(), bestConnector->id());
                m_lockedVehicleConnectorFromLinkIds.insert(
                    vehicle->id(), vehicle->roadId());
                if (lockedConnectorId <= 0 || connectorSelectionChanged)
                    m_lockedVehicleConnectorSelectedAtSec.insert(
                        vehicle->id(), m_lastStepSec);
                if (connectorReselected)
                    m_lockedVehicleConnectorReselections.insert(
                        vehicle->id(), m_lockedVehicleConnectorReselections
                            .value(vehicle->id(), 0) + 1);
            }
        }
        if (!laneChangeInitiationLocked && !laneChangeInProgress) {
            const bool linkLaneChangeSlotAvailable =
                activeLaneChangesByLink.value(currentLinkId, 0)
                    < maximumConcurrentLaneChangesPerLink
                && m_lastStepSec - m_lastLaneChangeStartSecByLink
                    .value(currentLinkId, -1e9)
                    >= minimumLaneChangeStartIntervalSecPerLink;
            const bool currentLaneCanStart =
                !queuedApproachLaneLock
                && vehicle->currSpeed(Metric) >= minimumMandatoryLaneChangeSpeedMps
                && currentLaneTraffic.occupancy
                    <= maximumMandatoryLaneChangeOccupancy;
            if (linkLaneChangeSlotAvailable && currentLaneCanStart) {
                const LaneBalanceAction laneAction = m_laneBalancer.choose(
                    vehicle->id(), currentLinkId, laneId, targetLinkId,
                    distanceToEnd, m_lastStepSec, m_traffic);
                if (laneAction.valid()) {
                    const LaneTraffic targetLaneTraffic = m_traffic.lanes()
                        .value(laneAction.targetLaneId);
                    const bool toLeft = laneAction.direction
                        == LaneChangeDirection::Left;
                    const double frontGapM = toLeft
                        ? vehicle->vehiDistLLaneFront(Metric)
                        : vehicle->vehiDistRLaneFront(Metric);
                    const double rearGapM = toLeft
                        ? vehicle->vehiDistLLaneRear(Metric)
                        : vehicle->vehiDistRLaneRear(Metric);
                    const double rearSpeedMps = toLeft
                        ? vehicle->vehiSpeedLLaneRear(Metric)
                        : vehicle->vehiSpeedRLaneRear(Metric);
                    const double requiredRearGapM = minimumMandatoryRearGapM
                        + qMax(0.0, rearSpeedMps - vehicle->currSpeed(Metric))
                            * rearClosingTimeBufferSec;
                    const bool frontGapClear = frontGapM <= 0.0
                        || frontGapM >= minimumMandatoryFrontGapM;
                    const bool rearGapClear = rearGapM <= 0.0
                        || rearGapM >= requiredRearGapM;
                    const bool targetLaneClear =
                        targetLaneTraffic.occupancy
                            <= maximumMandatoryTargetLaneOccupancy
                        && frontGapClear && rearGapClear;
                    if (targetLaneClear) {
                        command->setChangeLaneSafeDistance(
                            m2p(approachLaneChangeSafeDistanceM));
                        if (laneAction.mandatory && toLeft)
                            command->setLeftLaneChange(true);
                        else if (laneAction.mandatory)
                            command->setRightLaneChange(true);
                        else if (toLeft)
                            command->setLeftLaneChangeFreely(true);
                        else
                            command->setRightLaneChangeFreely(true);
                        ++activeLaneChangesByLink[currentLinkId];
                        m_lastLaneChangeStartSecByLink.insert(
                            currentLinkId, m_lastStepSec);
                    } else {
                        // Do not retry an unsafe mandatory movement inside the
                        // same approach. At the protection boundary the route
                        // recovery above chooses an OD-safe connector already
                        // reachable from the vehicle's current lane.
                        m_laneBalancer.cancelVehicleCommand(vehicle->id());
                    }
                }
            }
        }
        if (!signalLane) {
            m_signalHardStoppedVehicleIds.remove(vehicle->id());
            m_greenStoppedSinceSec.remove(vehicle->id());
            m_greenReleaseLastImpulseSec.remove(vehicle->id());
            m_greenReleaseImpulseAttempts.remove(vehicle->id());
            continue;
        }

        // Vehicle safety follows the SDK's executable physical color, while
        // SignalEngine remains the only writer.  This is the same contract as
        // Python: a pending command never authorizes a vehicle before TESSNG
        // has actually applied it.
        const QString actualColor = actualSignalColorForLane(laneId);
        const QString turnType = laneTurns.value(targetLinkId).toLower();
        const bool permissiveRight = permissiveRightTurn
            && turnType == "right";
        bool permissiveRightMustYield = false;
        if (permissiveRight) {
            const QVector<StageRef> movementRefs = m_model->movementStagesByLink()
                .value(vehicle->roadId()).value(targetLinkId);
            for (const StageRef& ref : movementRefs) {
                if (ref.intersection < 0
                    || ref.intersection >= m_signals.states().size())
                    continue;
                const IntersectionSignalState& signalState =
                    m_signals.states().at(ref.intersection);
                if (signalState.transition != SignalTransition::Green
                    || signalState.activeStage == ref.stage)
                    continue;
                const StageDefinition& activeStage = m_model->intersections()
                    .value(ref.intersection).stages.value(signalState.activeStage);
                bool protectsNonRightMovement = false;
                for (const QString& activeTurn : activeStage.turnTypes) {
                    if (activeTurn.toLower() != "right") {
                        protectsNonRightMovement = true;
                        break;
                    }
                }
                const double protectedQueue = m_traffic.stages()
                    .value(ref.intersection).value(signalState.activeStage).queueVeh;
                if (protectsNonRightMovement
                    && protectedQueue >= permissiveRightQueueThreshold) {
                    permissiveRightMustYield = true;
                    break;
                }
            }
        }
        if (permissiveRightMustYield
            && distanceToEnd >= -1.0 && distanceToEnd <= stopGuardM) {
            // Right turns remain signal-independent: they are held only as a
            // yield obligation while a conflicting protected stream has an
            // actual green and queued demand.  Native following/conflict logic
            // remains authoritative and no positive speed is forced.
            command->setDesirSpeed(0.0);
            m_permissiveRightYieldVehicleIds.insert(vehicle->id());
            continue;
        }
        const bool movementMayProceed = permissiveRight || actualColor == "G";
        if (!movementMayProceed && (actualColor == "R" || actualColor == "Y")
            && distanceToEnd >= -1.0 && distanceToEnd <= stopGuardM) {
            m_greenStoppedSinceSec.remove(vehicle->id());
            m_greenReleaseLastImpulseSec.remove(vehicle->id());
            command->setDesirSpeed(0.0);
            if (distanceToEnd <= hardStopM) {
                if (!m_signalHardStoppedVehicleIds.contains(vehicle->id()))
                    m_greenReleaseImpulseAttempts.insert(vehicle->id(), 0);
                command->setSpeed(0.0);
                m_signalHardStoppedVehicleIds.insert(vehicle->id());
            }
            continue;
        }
        if (movementMayProceed) {
            // IVehicle::Step batch commands use TESSNG's default pixel unit.
            // All controller values above are deliberately kept in SI units,
            // so convert only at the SDK boundary.
            command->setDesirSpeed(m2p(freeFlow));
            // A permissive right turn is not controlled by the signal, but it
            // still has to yield to vehicles already using the connector and
            // to the protected green stream.  Treating it as a green movement
            // here used to inject a positive speed every second and could keep
            // a conflicting connector occupied, making the real green stream
            // look permanently blocked.  Clear any stale signal hold and let
            // TESSNG's native conflict model decide when the right turn enters.
            if (permissiveRight) {
                m_signalHardStoppedVehicleIds.remove(vehicle->id());
                m_greenStoppedSinceSec.remove(vehicle->id());
                m_greenReleaseLastImpulseSec.remove(vehicle->id());
                m_greenReleaseImpulseAttempts.remove(vehicle->id());
                continue;
            }
            const bool wasHardStopped =
                m_signalHardStoppedVehicleIds.contains(vehicle->id());
            const double currentSpeed = vehicle->currSpeed(Metric);
            if (currentSpeed >= 0.35 || distanceToEnd < -1.0
                || distanceToEnd > releaseGuardM) {
                m_signalHardStoppedVehicleIds.remove(vehicle->id());
                m_greenStoppedSinceSec.remove(vehicle->id());
                m_greenReleaseLastImpulseSec.remove(vehicle->id());
                m_greenReleaseImpulseAttempts.remove(vehicle->id());
            } else if (distanceToEnd >= -1.0
                       && distanceToEnd <= releaseGuardM) {
                const bool isFirst =
                    vehicle->vehicleDriving()->isFirstVehi(true);
                IVehicle* frontVehicle = vehicle->vehicleFront();
                const double frontDistanceM = vehicle->vehiDistFront(Metric);
                const bool frontClear = !frontVehicle
                    || frontDistanceM >= releaseFrontClearanceM;
                // isFirstVehi can stay false while TESSNG still references a
                // departed/conflicting object. No actual front vehicle is a
                // stronger and safer indication that this vehicle leads its
                // approach.
                const bool releaseLeader = isFirst || !frontVehicle;
                bool physicalTargetAvailable = false;
                double minimumReceivingLaneOccupancy = 1.0;
                if (vehicle->lane()) {
                    for (ILaneConnector* connector
                         : vehicle->lane()->toLaneConnectors()) {
                        if (!connector || !connector->toLane()
                            || !connector->toLane()->link()
                            || connector->toLane()->link()->id()
                                != targetLinkId)
                            continue;
                        physicalTargetAvailable = true;
                        minimumReceivingLaneOccupancy = qMin(
                            minimumReceivingLaneOccupancy,
                            m_traffic.lanes().value(connector->toLane()->id())
                                .occupancy);
                    }
                }
                bool receivingCapacity = targetLinkId > 0
                    && physicalTargetAvailable
                    && minimumReceivingLaneOccupancy < releaseMaxOccupancy;
                QVector<StageRef> refs = m_model->movementStagesByLink()
                    .value(vehicle->roadId()).value(targetLinkId);
                if (refs.isEmpty())
                    refs = m_model->controlledIncomingStagesByLane().value(laneId);
                bool permissiveOpposingMovement = false;
                for (const StageRef& ref : refs) {
                    if (ref.intersection < 0 || ref.stage < 0) continue;
                    if (m_model->intersections().value(ref.intersection)
                            .stages.value(ref.stage).permissiveOpposing)
                        permissiveOpposingMovement = true;
                }
                // A blanket ban on release impulses for a paired circular-green
                // stage allowed the priority straight stream and the yielding
                // left turn to remain in mutual-yield deadlock. Release only
                // the straight stream; permissive left turns still rely solely
                // on TESSNG's native gap/conflict model.
                const bool hasProtectedPriority = !permissiveOpposingMovement
                    || turnType == QStringLiteral("straight");
                if (hasProtectedPriority
                    && releaseLeader && frontClear && receivingCapacity
                    && !m_greenStoppedSinceSec.contains(vehicle->id())) {
                    m_greenStoppedSinceSec.insert(vehicle->id(), m_lastStepSec);
                } else if (!hasProtectedPriority || !releaseLeader
                           || !frontClear || !receivingCapacity) {
                    m_greenStoppedSinceSec.remove(vehicle->id());
                    m_greenReleaseLastImpulseSec.remove(vehicle->id());
                }
                if (hasProtectedPriority
                    && isFirst && frontClear && receivingCapacity) {
                    QVariantMap detail;
                    detail.insert(QStringLiteral("linkId"), vehicle->roadId());
                    detail.insert(QStringLiteral("laneId"), laneId);
                    detail.insert(QStringLiteral("targetLinkId"), targetLinkId);
                    detail.insert(QStringLiteral("distanceToStopLineM"), distanceToEnd);
                    detail.insert(QStringLiteral("speedMps"), currentSpeed);
                    detail.insert(QStringLiteral("zeroSpeedSec"),
                                  vehicle->vehicleDriving()->zeroSpeedInterval() / 1000.0);
                    detail.insert(QStringLiteral("vehicleTypeCode"),
                                  vehicle->vehicleTypeCode());
                    detail.insert(QStringLiteral("frontVehicleId"),
                                  frontVehicle ? frontVehicle->id() : 0);
                    detail.insert(QStringLiteral("frontDistanceM"),
                                  frontDistanceM);
                    detail.insert(QStringLiteral("frontSpeedMps"),
                                  vehicle->vehiSpeedFront(Metric));
                    detail.insert(QStringLiteral("releaseLeader"),
                                  releaseLeader);
                    detail.insert(QStringLiteral("physicalTargetAvailable"),
                                  physicalTargetAvailable);
                    detail.insert(QStringLiteral("minimumReceivingLaneOccupancy"),
                                  minimumReceivingLaneOccupancy);
                    detail.insert(QStringLiteral("toLaneId"), vehicle->toLaneId());
                    IRouting* nativeRouting = vehicle->routing();
                    detail.insert(QStringLiteral("nativeRoutingId"),
                                  nativeRouting ? nativeRouting->id() : 0);
                    QStringList nativeRouteLinks;
                    if (nativeRouting) {
                        for (ILink* routeLink : nativeRouting->getLinks())
                            if (routeLink)
                                nativeRouteLinks.push_back(
                                    QString::number(routeLink->id()));
                    }
                    detail.insert(QStringLiteral("nativeRouteLinks"),
                                  nativeRouteLinks.join(','));
                    QStringList physicalConnectorTargets;
                    QStringList physicalConnectorIds;
                    if (vehicle->lane()) {
                        for (ILaneConnector* connector
                             : vehicle->lane()->toLaneConnectors()) {
                            if (!connector || !connector->toLane()
                                || !connector->toLane()->link())
                                continue;
                            physicalConnectorIds.push_back(
                                QString::number(connector->id()));
                            physicalConnectorTargets.push_back(
                                QString::number(connector->toLane()->link()->id()));
                        }
                    }
                    detail.insert(QStringLiteral("physicalLaneConnectorIds"),
                                  physicalConnectorIds.join(','));
                    detail.insert(QStringLiteral("physicalLaneConnectorTargets"),
                                  physicalConnectorTargets.join(','));
                    double maximumDownstreamOccupancy = 0.0;
                    QStringList intersectionIds;
                    for (const StageRef& ref : refs) {
                        if (ref.intersection < 0 || ref.stage < 0) continue;
                        maximumDownstreamOccupancy = qMax(
                            maximumDownstreamOccupancy,
                            m_traffic.stages().value(ref.intersection)
                                .value(ref.stage).downstreamOccupancy);
                        const QString intersectionId =
                            m_model->intersections().value(ref.intersection).id;
                        if (!intersectionId.isEmpty()
                            && !intersectionIds.contains(intersectionId))
                            intersectionIds.push_back(intersectionId);
                    }
                    detail.insert(QStringLiteral("intersectionIds"),
                                  intersectionIds.join(','));
                    detail.insert(QStringLiteral("downstreamOccupancy"),
                                  maximumDownstreamOccupancy);
                    detail.insert(QStringLiteral("turnType"), turnType);
                    detail.insert(QStringLiteral("protectedPriorityRelease"),
                                  hasProtectedPriority);
                    m_greenPendingDetails.insert(vehicle->id(), detail);
                } else {
                    m_greenPendingDetails.remove(vehicle->id());
                }
                const bool releaseTimedOut = m_lastStepSec
                    - m_greenStoppedSinceSec.value(vehicle->id(), m_lastStepSec)
                    >= 1.0;
                const bool impulseDue = !m_greenReleaseLastImpulseSec
                    .contains(vehicle->id())
                    || m_lastStepSec - m_greenReleaseLastImpulseSec
                        .value(vehicle->id(), -1e9) >= 1.0;
                if (hasProtectedPriority
                    && receivingCapacity
                    && releaseLeader
                    && frontClear
                    && (wasHardStopped || releaseTimedOut)
                    && impulseDue
                    && m_greenReleaseImpulseAttempts.value(
                        vehicle->id(), 0)
                        < maximumGreenReleaseImpulseAttempts) {
                    command->setSpeed(m2p(qMax(currentSpeed, releaseSpeedMps)));
                    m_greenReleaseLastImpulseSec.insert(
                        vehicle->id(), m_lastStepSec);
                    m_greenReleaseImpulseAttempts.insert(vehicle->id(),
                        m_greenReleaseImpulseAttempts.value(vehicle->id(), 0) + 1);
                    m_signalHardStoppedVehicleIds.remove(vehicle->id());
                    ++m_greenReleaseImpulseCount;
                    // One bounded wake-up clears a stale hard-stop command.
                    // Repeated setSpeed impulses fight TESSNG's native
                    // following/conflict model and appear as red/blue speed
                    // flicker when the vehicle is still legitimately blocked.
                }
            }
            continue;
        }
        if (cavConfig.value("speedGuidanceEnabled").toBool(true)
            && distanceToEnd >= 0.0
            && distanceToEnd <= cavConfig.value("guidanceRadiusM").toDouble(180.0)) {
            const double wait = m_signals.waitForMovement(
                vehicle->roadId(), targetLinkId, m_lastStepSec);
            command->setDesirSpeed(m2p(qBound(minimumAdvice,
                distanceToEnd / qMax(1.0, wait + 1.0), freeFlow)));
        }
    }
    for (auto it = m_signalHardStoppedVehicleIds.begin();
         it != m_signalHardStoppedVehicleIds.end();) {
        if (!liveVehicleIds.contains(*it)) it = m_signalHardStoppedVehicleIds.erase(it);
        else ++it;
    }
    for (auto it = m_greenStoppedSinceSec.begin();
         it != m_greenStoppedSinceSec.end();) {
        if (!liveVehicleIds.contains(it.key()))
            it = m_greenStoppedSinceSec.erase(it);
        else {
            m_maximumGreenReleaseWaitSec=qMax(m_maximumGreenReleaseWaitSec,
                m_lastStepSec-it.value());
            ++it;
        }
    }
    for (auto it = m_greenReleaseLastImpulseSec.begin();
         it != m_greenReleaseLastImpulseSec.end();) {
        if (!liveVehicleIds.contains(it.key()))
            it = m_greenReleaseLastImpulseSec.erase(it);
        else
            ++it;
    }
    for (auto it = m_greenReleaseImpulseAttempts.begin();
         it != m_greenReleaseImpulseAttempts.end();) {
        if (!liveVehicleIds.contains(it.key()))
            it = m_greenReleaseImpulseAttempts.erase(it);
        else
            ++it;
    }
    for (auto it = m_greenPendingDetails.begin();
         it != m_greenPendingDetails.end();) {
        if (!m_greenStoppedSinceSec.contains(it.key()))
            it = m_greenPendingDetails.erase(it);
        else
            ++it;
    }
    for (auto it = m_lastVehicleLinkIds.begin(); it != m_lastVehicleLinkIds.end();) {
        if (!liveVehicleIds.contains(it.key())) it = m_lastVehicleLinkIds.erase(it);
        else ++it;
    }
    for (auto it = m_connectorDeadlockRecoveryVehicleIds.begin();
         it != m_connectorDeadlockRecoveryVehicleIds.end();) {
        if (!liveVehicleIds.contains(*it)) {
            m_connectorDeadlockLastImpulseSec.remove(*it);
            m_connectorDeadlockImpulseAttempts.remove(*it);
            it = m_connectorDeadlockRecoveryVehicleIds.erase(it);
        } else
            ++it;
    }
    for (auto it = m_lockedVehicleConnectorIds.begin();
         it != m_lockedVehicleConnectorIds.end();) {
        if (!liveVehicleIds.contains(it.key())) {
            m_lockedVehicleConnectorFromLinkIds.remove(it.key());
            m_lockedVehicleConnectorSelectedAtSec.remove(it.key());
            m_lockedVehicleConnectorReselections.remove(it.key());
            it = m_lockedVehicleConnectorIds.erase(it);
        } else {
            ++it;
        }
    }
}

void MySimulator::sampleTrajectories(const QList<IVehicle*>& vehicles)
{
    QSet<qint64> sampled;
    sampled.reserve(m_maximumDisplayedVehicles);
    for (IVehicle* vehicle : vehicles) {
        if (!vehicle) continue;
        const qint64 vehicleId = vehicle->id();
        if (!m_trajectories.contains(vehicleId)
            && sampled.size() >= m_maximumDisplayedVehicles)
            continue;
        const QPointF position = vehicle->pos(Metric);
        QVariantMap point;
        point.insert("xM", position.x());
        point.insert("yM", position.y());
        point.insert("speedKph", vehicle->currSpeed(Metric) * 3.6);
        point.insert("laneId", static_cast<qlonglong>(vehicle->laneId()));
        QVector<QVariantMap>& trail = m_trajectories[vehicleId];
        trail.push_back(point);
        if (trail.size() > m_trajectoryRetentionPoints)
            trail.remove(0, trail.size() - m_trajectoryRetentionPoints);
        sampled.insert(vehicleId);
    }
    for (auto it = m_trajectories.begin(); it != m_trajectories.end();) {
        if (!sampled.contains(it.key())) it = m_trajectories.erase(it);
        else ++it;
    }
}

void MySimulator::publishTelemetry()
{
    const NetworkTelemetry& data = m_traffic.telemetry();
    const int index = qBound(0, m_selectedIntersection,
                             qMax(0, m_model->intersections().size() - 1));
    QVariantMap values;
    values["time"] = data.simulationTimeSec;
    values["running"] = data.runningVehicles;
    values["completed"] = data.completedVehicles;
    values["speed"] = data.meanSpeedKph;
    values["queue"] = data.queuedVehicles;
    values["energy"] = data.energyKWh;
    values["multiple"] = data.actualMultiple;
    values["targetMultiple"] = m_targetMultiple;
    values["schedulerMultiple"] = m_schedulerMultiple;
    values["speedCapacityLimited"] = m_speedCapacityLimited;
    values["actualCompletedVehicles"] =
        m_metrics.eligibleCompletedVehicleCount();
    values["completedDelaySec"] = m_metrics.meanCompletedDelaySec();
    values["completedDelaySamples"] = m_metrics.completedDelaySampleCount();
    qint64 signalSwitchCount = 0;
    for (const IntersectionSignalState& signalState : m_signals.states()) {
        qint64 serviceCount = 0;
        for (int count : signalState.stageServiceCount)
            serviceCount += count;
        signalSwitchCount += qMax<qint64>(0, serviceCount - 1);
    }
    SimuInterface* simulation = gpTessInterface->simuInterface();
    values["runState"] = simulation && simulation->isPausing()
        ? QStringLiteral("仿真已暂停")
        : simulation && simulation->isRunning()
            ? QStringLiteral("仿真运行中")
            : QStringLiteral("等待手动开始");
    values["rsuMode"] = QStringLiteral("VIRTUAL_RSU_TESSNG");
    values["sampleCount"] = m_telemetrySampleCount;
    values["signalSwitchCount"] = signalSwitchCount;
    values["controlledSignalLampCount"] = m_signalLamps.size();
    values["recoveredMovementCount"] = m_recoveredMovementCount;
    values["greenReleaseImpulseCount"] = m_greenReleaseImpulseCount;
    values["connectorDeadlockReleaseImpulseCount"] =
        m_connectorDeadlockReleaseImpulseCount;
    values["stopLineLaneChangeDismissVehicles"] =
        m_stopLineLaneChangeDismissVehicleIds.size();
    values["pendingGreenReleaseVehicles"] = m_greenStoppedSinceSec.size();
    values["stage"] = m_signals.activeStageId(index);
    values["owner"] = m_signals.authorityOwner(index);
    values["signalReadbackAbnormalIntersections"] =
        m_signalReadbackAbnormalIntersections;
    values["signalReadbackTimeSec"] = m_signalReadbackTimeSec;

    const IntersectionDefinition& intersection = m_model->intersections()[index];
    const QVector<StageTraffic> localStages = m_traffic.stages().value(index);
    double localQueue = 0.0;
    double localVehicles = 0.0;
    double localSpeedWeighted = 0.0;
    double localArrivals = 0.0;
    QVariantList stageValues;
    for (int stageIndex = 0; stageIndex < intersection.stages.size(); ++stageIndex) {
        const StageTraffic traffic = localStages.value(stageIndex);
        localQueue += traffic.queueVeh;
        localVehicles += traffic.vehicleCount;
        localSpeedWeighted += traffic.meanSpeedMps * traffic.vehicleCount;
        localArrivals += traffic.arrivalsVehPerMin;
        QVariantMap stage;
        stage.insert("stageId", intersection.stages[stageIndex].id);
        stage.insert("queueVeh", traffic.queueVeh);
        stage.insert("vehicleCount", traffic.vehicleCount);
        stage.insert("meanSpeedKph", traffic.meanSpeedMps * 3.6);
        stage.insert("arrivalsVehPerMin", traffic.arrivalsVehPerMin);
        stage.insert("downstreamQueueVeh", traffic.downstreamQueueVeh);
        stage.insert("occupancy", traffic.downstreamOccupancy);
        stage.insert("color", m_signals.colorForStage(index, stageIndex));
        stageValues.push_back(stage);
    }
    values["selectedIndex"] = index;
    values["intersectionId"] = intersection.id;
    values["intersectionName"] = intersection.name;
    values["junctionTypeName"] = intersection.junctionTypeName;
    values["elevationLayerName"] = intersection.elevationLayerName;
    values["connectorAreaId"] = static_cast<qlonglong>(intersection.connectorAreaId);
    values["armCount"] = intersection.armCount;
    values["centerX"] = intersection.centerM.x();
    values["centerY"] = intersection.centerM.y();
    values["intersectionQueue"] = localQueue;
    values["intersectionVehicles"] = localVehicles;
    values["intersectionSpeed"] = localVehicles > 0.0
        ? localSpeedWeighted * 3.6 / localVehicles : 0.0;
    values["intersectionArrivals"] = localArrivals;
    values["controlMode"] = algorithmName();
    values["signalized"] = intersection.signalized;
    values["permanentGreen"] = intersection.permanentGreen;
    QVariantList mergedMemberIds;
    for (const QString& memberId : intersection.mergedMemberIds)
        mergedMemberIds.push_back(memberId);
    values["mergedMemberIds"] = mergedMemberIds;
    values["mergedIntersection"] = !intersection.mergedMemberIds.isEmpty();
    values["stages"] = stageValues;

    const IntersectionSignalState state = m_signals.states().value(index);
    values["phaseElapsed"] = qMax(0.0, data.simulationTimeSec - state.stateStartedSec);
    values["leaseUntil"] = state.authorityLeaseUntilSec;
    QString signalColor = "UNSIGNALIZED";
    if (intersection.signalized) {
        signalColor = "RED";
        if (intersection.permanentGreen) signalColor = "GREEN";
        else if (state.transition == SignalTransition::AllRed) signalColor = "ALL_RED";
        else if (state.transition == SignalTransition::Yellow) signalColor = "YELLOW";
        else signalColor = "GREEN";
    }
    values["signalColor"] = signalColor;

    QVariantMap trajectories;
    for (auto it = m_trajectories.cbegin(); it != m_trajectories.cend(); ++it) {
        QVariantList points;
        points.reserve(it.value().size());
        for (const QVariantMap& point : it.value()) points.push_back(point);
        trajectories.insert(QString::number(it.key()), points);
    }
    values["trajectories"] = trajectories;
    values["trajectoryVehicleCount"] = trajectories.size();
    emit telemetryUpdated(values);
}

void MySimulator::updateClosedLoopSpeed(double simulationTimeSec)
{
    const QJsonObject config = m_model->scenario().value("cav").toObject()
        .value("closedLoopSpeed").toObject();
    if (!m_speedClock.isValid()) m_speedClock.start();
    const double wallSec = m_speedClock.elapsed() / 1000.0;
    if (!m_speedMeasurements.isEmpty()) {
        const QPair<double, double> last = m_speedMeasurements.last();
        if (simulationTimeSec < last.second || wallSec - last.first > 2.0) {
            m_speedMeasurements.clear();
            m_actualMultiple = 0.0;
        } else if (wallSec - last.first < 0.25) return;
    }
    m_speedMeasurements.push_back(qMakePair(wallSec, simulationTimeSec));
    while (m_speedMeasurements.size() > 2
           && wallSec - m_speedMeasurements.first().first > 4.0)
        m_speedMeasurements.removeFirst();
    if (m_speedMeasurements.size() < 2) return;
    const double wallDelta = wallSec - m_speedMeasurements.first().first;
    if (wallDelta < qMax(0.5,
            config.value("minimumObservationWallSec").toDouble(1.5))) return;
    const double measured = qMax(0.0,
        (simulationTimeSec - m_speedMeasurements.first().second) / wallDelta);
    if (measured <= 0.0) return;
    const double alpha = qBound(0.05,
        config.value("measurementEmaAlpha").toDouble(0.35), 1.0);
    m_actualMultiple = m_actualMultiple > 0.0
        ? (1.0 - alpha) * m_actualMultiple + alpha * measured
        : measured;

    // acceMultiplesReally() is an instantaneous SDK diagnostic and can return
    // implausible spikes at high scheduler multiples. Wall-clock progress is
    // authoritative for display, result files and closed-loop feedback.
    if (!config.value("enabled").toBool(true)) return;
    const double actual = m_actualMultiple;
    const double target = m_targetMultiple;
    const double toleranceRatio = qBound(0.01,
        config.value("toleranceRatio").toDouble(0.08), 0.30);
    const double error = target - actual;
    if (qAbs(error) <= target * toleranceRatio) {
        m_speedIntegralError *= 0.5;
        m_underTargetCount = 0;
        m_capacityLimitCount = 0;
        m_speedCapacityLimited = false;
        return;
    }
    const double interval = qMax(0.25,
        config.value("adjustmentIntervalWallSec").toDouble(2.0));
    if (wallSec - m_lastSpeedAdjustWallSec < interval) return;
    m_lastSpeedAdjustWallSec = wallSec;
    const int minimum = qMax(1,
        config.value("minimumCommandMultiple").toInt(1));
    const int maximum = qMax(minimum,
        config.value("maximumCommandMultiple").toInt(50));
    const int maximumStep = qMax(1,
        config.value("maximumCommandStep").toInt(12));
    const double proportional = qMax(0.05,
        config.value("proportionalGain").toDouble(0.70));
    const double integralGain = qMax(0.0,
        config.value("integralGain").toDouble(0.08));
    m_speedIntegralError = qBound(-double(maximum),
        m_speedIntegralError + error * interval, double(maximum));
    int candidate = m_schedulerMultiple;
    if (error > 0.0) {
        ++m_underTargetCount;
        const int feedback = qMin(maximumStep, qMax(1,
            qRound(proportional * error + integralGain * m_speedIntegralError)));
        const int feedForward = qRound(m_schedulerMultiple
            * target / qMax(actual, 0.05));
        candidate = qMin(m_schedulerMultiple + maximumStep,
            qMax(m_schedulerMultiple + feedback, feedForward));
    } else {
        m_underTargetCount = 0;
        const int feedback = qMin(maximumStep, qMax(1,
            qRound(proportional * qAbs(error))));
        const int feedForward = qRound(m_schedulerMultiple
            * target / qMax(actual, 0.05));
        candidate = qMax(m_schedulerMultiple - maximumStep,
            qMin(m_schedulerMultiple - feedback, feedForward));
    }
    candidate = qBound(minimum, candidate, maximum);
    if (candidate != m_schedulerMultiple) {
        m_schedulerMultiple = candidate;
        gpTessInterface->simuInterface()->setAcceMultiples(candidate);
    }
    const bool commandAtLimit = m_schedulerMultiple >= maximum;
    const bool materiallyBelowTarget = actual < target * (1.0 - toleranceRatio);
    if (commandAtLimit && materiallyBelowTarget)
        ++m_capacityLimitCount;
    else
        m_capacityLimitCount = 0;
    const int confirmations = qMax(1,
        config.value("capacityLimitConfirmations").toInt(3));
    const bool limited = m_capacityLimitCount >= confirmations;
    if (limited != m_speedCapacityLimited) {
        m_speedCapacityLimited = limited;
        if (limited) {
            emit eventGenerated(QStringLiteral(
                "倍速已达稳定运行上限：目标 %1×，墙钟实测 %2×，TESSNG安全调度 %3×；继续提高目标值不会提升当前吞吐量")
                .arg(target).arg(actual, 0, 'f', 2).arg(m_schedulerMultiple));
        }
    }
}

void MySimulator::afterOneStep()
{
    QMutexLocker callbackGuard(&m_nativeCallbackMutex);
    SimuInterface* simulation = gpTessInterface->simuInterface();
    const double nowSec = simulation->simuTimeIntervalWithAcceMutiples() / 1000.0;
    const double deltaSec = m_lastStepSec > 0.0 ? qMax(0.0, nowSec - m_lastStepSec) : 0.1;
    m_lastStepSec = nowSec;
    m_routing.setSimulationTime(nowSec);
    updateClosedLoopSpeed(nowSec);

    m_demand.step(nowSec, deltaSec, simulation, &m_routing);
    const QList<IVehicle*> vehicles = simulation->allVehiStarted();

    if (nowSec - m_lastAggregateSec >= 0.999) {
        m_lastAggregateSec = nowSec;
        m_traffic.sample(vehicles, nowSec, simulation->vehiCountTotal(),
                         m_actualMultiple);
        sampleTrajectories(vehicles);
    }
    if (nowSec - m_lastRouteRebuildSec >= 4.999) {
        m_lastRouteRebuildSec = nowSec;
        m_routing.rebuildShortestPathForests(m_traffic, &m_signals, nowSec);
    }
    // Observe the previously submitted frame before issuing this batch.  A
    // one-second settling window prevents normal transitions being reported
    // as faults and matches the Python signal-output contract.
    verifySignalReadback(nowSec);
    m_signals.update(nowSec, m_traffic);
    writeSignalFrame();
    executeVehicleSafety(vehicles);
    m_traffic.setRuntimeDiagnostics(
        m_wrongExitBlockedVehicleIds.size(),
        m_greenStoppedSinceSec.size(),
        m_connectorStallDetails.size(),
        m_routeMaintenanceFailures.size(),
        m_stopLineLaneChangeDismissVehicleIds.size(),
        m_greenReleaseImpulseCount,
        m_connectorDeadlockReleaseImpulseCount);
    if (qAbs(nowSec - m_lastAggregateSec) < 0.001)
        m_metrics.observe(vehicles, m_traffic, m_routing, m_signals, nowSec);
    if (qAbs(nowSec - m_lastAggregateSec) < 0.001)
        m_metrics.append(m_traffic.telemetry());
    if (nowSec - m_lastTelemetrySec >= 0.999) {
        m_lastTelemetrySec = nowSec;
        ++m_telemetrySampleCount;
        publishTelemetry();
    }
    if (runtimeSmokeTestRequested()
        && nowSec >= runtimeSmokeTestDurationSec()) {
        const bool success = m_signalLamps.size() == m_model->expectedSignalLampCount()
            && m_laneBalancer.wideningUnresolvedCount() == 0
            && m_demand.preloadCreated() > 0
            && m_routing.vehicleCountWithOd() > 0
            && m_routing.routeDecisionCount() > 0
            && m_routeMaintenanceFailures.isEmpty()
            && m_metrics.wrongDestinationVehicleCount() == 0
            && m_maximumGreenReleaseWaitSec <= 3.0
            && m_signalReadbackAbnormalIntersections == 0;
        finishRuntimeSmokeTest(success, success ? QString() : QStringLiteral(
            "runtime callback, demand, routing, or signal readback criterion failed"));
        simulation->stopSimu();
        return;
    }
    if (nowSec >= m_durationSec + 15.0) simulation->stopSimu();
}

void MySimulator::initVehicle(IVehicle* vehicle)
{
    if (!vehicle) return;
    const int callbackSteps = qMax(1, m_model->scenario()
        .value("cav").toObject().value("laneBalancing").toObject()
        .value("nativeLaneChangeCallbackSteps").toInt(1));
    vehicle->setSteps_calcLimitedLaneNumber(callbackSteps);
    vehicle->setSteps_calcChangeLaneSafeDist(callbackSteps);
    m_routing.registerVehicle(vehicle);
}

bool MySimulator::calcLampColor(ISignalLamp* signalLamp)
{
    if (!signalLamp || !signalLamp->laneObject() || !signalLamp->laneObject()->isLane()) return false;
    const qint64 laneId = signalLamp->laneObject()->id();
    if (!m_model->controlledIncomingStagesByLane().contains(laneId)) return false;
    signalLamp->setLampColor(m_signals.colorForLane(laneId));
    return true;
}

void MySimulator::afterDistriRouting(IVehicle* vehicle,
                                    IDecisionPoint* decisionPoint,
                                    IRouting* routing)
{
    m_routing.applyDecision(vehicle, decisionPoint, routing);
}

QList<ILaneConnector*> MySimulator::candidateLaneConnectors(
    IVehicle* vehicle, QList<ILaneConnector*> candidates)
{
    // executeVehicleSafety selects one receiving lane using current occupancy.
    // This high-frequency SDK callback used to run a second independent route
    // selection and could expose a different connector at the stop line. Honor
    // the per-link commitment here so native execution has exactly one answer.
    QMutexLocker callbackGuard(&m_nativeCallbackMutex);
    if (vehicle && vehicle->roadIsLink()
        && m_lockedVehicleConnectorFromLinkIds.value(vehicle->id(), 0)
            == vehicle->roadId()) {
        const qint64 lockedConnectorId =
            m_lockedVehicleConnectorIds.value(vehicle->id(), 0);
        for (ILaneConnector* connector : candidates) {
            if (connector && connector->id() == lockedConnectorId) {
                QList<ILaneConnector*> committed;
                committed.push_back(connector);
                return committed;
            }
        }
        // The vehicle may have completed an upstream lane change since the
        // lock was created. Discard only a physically absent lock and let the
        // routing boundary choose from the new lane's real connectors.
        m_lockedVehicleConnectorIds.remove(vehicle->id());
        m_lockedVehicleConnectorFromLinkIds.remove(vehicle->id());
        m_lockedVehicleConnectorSelectedAtSec.remove(vehicle->id());
        m_lockedVehicleConnectorReselections.remove(vehicle->id());
    }
    return m_routing.filterLaneConnectors(vehicle, candidates);
}

bool MySimulator::shouldFreezeApproachLane(IVehicle* vehicle) const
{
    if (!m_model || !vehicle || !vehicle->roadIsLink()
        || !vehicle->vehicleDriving())
        return false;
    const QJsonObject laneBalancing = m_model->scenario().value("cav")
        .toObject().value("laneBalancing").toObject();
    if (!laneBalancing.value(
            "hardFreezeCurrentLaneInProtectedZone").toBool(true))
        return false;
    if (!m_model->incomingStagesByLink().contains(vehicle->roadId()))
        return false;
    const double distanceToEndM = vehicle->vehicleDriving()->distToEndpoint(
        true, true, Metric);
    // Use the same upstream initiation boundary as the route/lane balancer.
    // The previous 90 m native boundary still allowed TESSNG itself to start
    // a manoeuvre between 90 m and 220 m; in a queue it could not finish and
    // left the vehicle diagonal at the stop line. Existing traces are exempt
    // below and are never cancelled, so this wider boundary only prevents a
    // new late manoeuvre.
    const double hardFreezeDistanceM = qMax(
        qMax(qMax(10.0, laneBalancing.value(
            "freezeDistanceBeforeStopLineM").toDouble(60.0)),
            laneBalancing.value("queueLaneLockDistanceM").toDouble(90.0)),
        laneBalancing.value("laneChangeInitiationCutoffBeforeStopLineM")
            .toDouble(220.0));
    // Position is monotonic on an approach, unlike instantaneous speed. Once
    // inside this final zone the native restriction therefore stays stable
    // until the vehicle enters the connector.
    return distanceToEndM >= -1.0 && distanceToEndM <= hardFreezeDistanceM;
}

QList<int> MySimulator::calcLimitedLaneNumber(IVehicle* vehicle)
{
    QList<int> prohibited;
    if (!shouldFreezeApproachLane(vehicle) || !vehicle->lane()
        || !vehicle->lane()->link() || !vehicle->vehicleDriving())
        return prohibited;
    // Restrict only before a manoeuvre starts. If the SDK has already passed
    // its one-third cancellation boundary, banning the target lane would
    // strand the vehicle diagonally; leave it free to complete instead.
    if (!vehicle->vehicleDriving()->changingTrace(Metric).isEmpty())
        return prohibited;
    const int currentLaneNumber = vehicle->vehicleDriving()->laneNumber();
    const int laneCount = vehicle->lane()->link()->laneCount();
    for (int number = 0; number < laneCount; ++number)
        if (number != currentLaneNumber) prohibited.push_back(number);
    return prohibited;
}

bool MySimulator::reCalcDismissChangeLane(IVehicle* vehicle)
{
    if (!shouldFreezeApproachLane(vehicle)) return false;
    QMutexLocker callbackGuard(&m_nativeCallbackMutex);
    m_laneBalancer.cancelVehicleCommand(vehicle->id());
    // Never cancel a trace already in progress. calcLimitedLaneNumber prevents
    // a new change; returning true here interrupts an existing trace and can
    // strand the vehicle diagonally over adjacent lanes at the stop line.
    return false;
}

bool MySimulator::calcChangeLaneSafeDist(IVehicle* vehicle, qreal& distance)
{
    UnitOfMeasure unit = Metric;
    if (!calcChangeLaneSafeDist(vehicle, distance, &unit)) return false;
    distance = m2p(distance);
    return true;
}

bool MySimulator::calcChangeLaneSafeDist(IVehicle* vehicle, qreal& distance,
                                         UnitOfMeasure* unit)
{
    if (!vehicle || !vehicle->roadIsLink() || !vehicle->vehicleDriving())
        return false;
    const QJsonObject laneBalancing = m_model->scenario().value("cav")
        .toObject().value("laneBalancing").toObject();
    const double protectionDistanceM = qMax(
        laneBalancing.value("laneChangeInitiationCutoffBeforeStopLineM")
            .toDouble(220.0),
        laneBalancing.value("connectorSelectionLookaheadM").toDouble(320.0));
    const bool intersectionApproach =
        m_model->incomingStagesByLink().contains(vehicle->roadId());
    if (!intersectionApproach
        || vehicle->vehicleDriving()->distToEndpoint(true, true, Metric)
            > protectionDistanceM)
        return false;
    distance = qMax(0.0, laneBalancing.value(
        "approachLaneChangeSafeDistanceM").toDouble(12.0));
    if (unit) *unit = Metric;
    return true;
}

void MySimulator::beforeToLeftFreely(IVehicle* vehicle, bool& keepOn)
{
    const QJsonObject laneBalancing = m_model->scenario().value("cav")
        .toObject().value("laneBalancing").toObject();
    const double freezeDistanceM = qMax(
        qMax(qMax(10.0, laneBalancing.value(
            "freezeDistanceBeforeStopLineM").toDouble(60.0)),
            laneBalancing.value("queueLaneLockDistanceM").toDouble(90.0)),
        laneBalancing.value("laneChangeInitiationCutoffBeforeStopLineM")
            .toDouble(220.0));
    if (vehicle && vehicle->roadIsLink() && vehicle->vehicleDriving()
        && (m_laneBalancer.isLaneChangeLocked(vehicle->roadId())
            || (m_model->incomingStagesByLink().contains(vehicle->roadId())
                && vehicle->vehicleDriving()->distToEndpoint(true, true, Metric)
                    < freezeDistanceM)))
        keepOn = false;
}

void MySimulator::beforeToRightFreely(IVehicle* vehicle, bool& keepOn)
{
    beforeToLeftFreely(vehicle, keepOn);
}

void MySimulator::leaveForNextLaneObj(IVehicle* vehicle, ILaneObject* current,
                                      ILaneObject* next)
{
    QMutexLocker callbackGuard(&m_nativeCallbackMutex);
    if (vehicle && !next) {
        const VehicleOdState od=m_routing.vehicleOdState(vehicle->id());
        qint64 lastLink = 0;
        // The callback arguments are authoritative at the road boundary.
        // vehicle->roadId() may already refer to the connector being left,
        // while the 10 Hz snapshot can still contain the preceding link.
        if (current && current->isLane()) {
            ILane* lane = current->castToLane();
            if (lane && lane->link()) lastLink = lane->link()->id();
        } else if (current) {
            ILaneConnector* connector = current->castToLaneConnector();
            if (connector && connector->toLane()
                && connector->toLane()->link())
                lastLink = connector->toLane()->link()->id();
        }
        if (lastLink <= 0 && vehicle->roadIsLink())
            lastLink = vehicle->roadId();
        if (lastLink <= 0)
            lastLink = m_lastVehicleLinkIds.value(vehicle->id(),0);
        const double now=gpTessInterface->simuInterface()
            ->simuTimeIntervalWithAcceMutiples()/1000.0;
        m_metrics.finishVehicle(vehicle->id(),now,lastLink==od.destinationExitLinkId,lastLink);
        m_routing.unregisterVehicle(vehicle->id());
        m_lastVehicleLinkIds.remove(vehicle->id());
    }
}

void MySimulator::requestStart()
{
    SimuInterface* simulation = gpTessInterface->simuInterface();
    if (simulation->isPausing()) simulation->pauseSimuOrNot();
    else if (!simulation->isRunning()) simulation->startSimu();
}

void MySimulator::requestPause()
{
    gpTessInterface->simuInterface()->pauseSimuOrNot();
}

void MySimulator::requestReset()
{
    if (m_comparisonActive || m_comparisonPendingStart) {
        emit eventGenerated(QStringLiteral(
            "自动对照进行中，不能手动重置子实验；可暂停/继续或停止自动对照"));
        return;
    }
    SimuInterface* simulation = gpTessInterface->simuInterface();
    if (simulation->isRunning() || simulation->isPausing()) {
        m_restartAfterStop = true;
        simulation->stopSimu();
    } else {
        m_restartAfterStop = false;
        QTimer::singleShot(0, this, [simulation]() { simulation->startSimu(); });
    }
}

void MySimulator::requestStop()
{
    if (m_comparisonActive || m_comparisonPendingStart) {
        requestCancelComparison();
        return;
    }
    m_restartAfterStop = false;
    gpTessInterface->simuInterface()->stopSimu();
}

void MySimulator::setAlgorithm(const QString& algorithm)
{
    if (m_comparisonActive || m_comparisonPendingStart) {
        emit eventGenerated(QStringLiteral("自动对照进行中，算法由批次调度器自动切换"));
        return;
    }
    m_requestedBranch = algorithm == "FIXED_TIMING"
        ? ControlBranch::FixedTiming : ControlBranch::ImprovedMaxPressure;
    if (!m_running) m_signals.setBranch(m_requestedBranch);
    const QString selected = m_requestedBranch == ControlBranch::FixedTiming
        ? "FIXED_TIMING" : "IMPROVED_MAX_PRESSURE";
    emit eventGenerated(QStringLiteral("算法已选择为 %1；将在下一轮0秒生效")
        .arg(selected));
}

void MySimulator::setTargetMultiple(int multiple)
{
    const int previousTarget = m_targetMultiple;
    m_targetMultiple = qBound(1, multiple, 50);
    const QJsonObject config = m_model->scenario().value("cav").toObject()
        .value("closedLoopSpeed").toObject();
    const int maximumCommand = qMax(1,
        config.value("maximumCommandMultiple").toInt(50));
    m_schedulerMultiple = m_running && previousTarget > 0
        ? qBound(1, qRound(double(m_schedulerMultiple)
            * m_targetMultiple / previousTarget), maximumCommand)
        : m_targetMultiple;
    gpTessInterface->simuInterface()->setAcceMultiples(m_schedulerMultiple);
    m_speedMeasurements.clear();
    m_actualMultiple = 0.0;
    m_speedIntegralError = 0.0;
    m_underTargetCount = 0;
    m_capacityLimitCount = 0;
    m_speedCapacityLimited = false;
    m_lastSpeedAdjustWallSec = -1e9;
    m_speedClock.restart();
}

void MySimulator::setOuterRate(int rate)
{
    m_demand.setOuterRate(rate);
}

void MySimulator::setResidentialRate(int rate)
{
    m_demand.setResidentialRate(rate);
}

void MySimulator::setDurationSec(int seconds)
{
    m_durationSec = qBound(60, seconds, 86400);
    gpTessInterface->simuInterface()->setSimuIntervalScheming((m_durationSec + 15) * 1000L);
}

void MySimulator::setRandomSeeds(int vehicleSeed, int destinationSeed)
{
    if (m_comparisonActive || m_comparisonPendingStart) {
        emit eventGenerated(QStringLiteral("自动对照进行中，同轮种子已锁定"));
        return;
    }
    m_requestedVehicleSeed = qBound(0, vehicleSeed, 2147483647);
    m_requestedDestinationSeed = qBound(0, destinationSeed, 2147483647);
    emit eventGenerated(QStringLiteral("随机种子已设为 %1 / %2；将在下一轮0秒生效")
        .arg(m_requestedVehicleSeed).arg(m_requestedDestinationSeed));
}

void MySimulator::setPreloadCount(int count)
{
    m_requestedPreloadCount = qBound(
        0, count, m_model ? m_model->placements().size() : 0);
    emit eventGenerated(QStringLiteral("预加载车辆数已设为 %1；将在下一轮0秒生效")
        .arg(m_requestedPreloadCount));
}

void MySimulator::setSelectedIntersection(int index)
{
    m_selectedIntersection = index;
    publishTelemetry();
}

void MySimulator::requestComparison(int rounds)
{
    if (runtimeSmokeTestRequested()) return;
    if (m_comparisonActive || m_comparisonPendingStart) {
        emit eventGenerated(QStringLiteral("自动对照已在运行"));
        return;
    }
    m_comparisonRoundsTotal = qBound(1, rounds, 100);
    m_manualBranchBeforeComparison = m_requestedBranch;
    m_manualVehicleSeedBeforeComparison = m_requestedVehicleSeed;
    m_manualDestinationSeedBeforeComparison = m_requestedDestinationSeed;
    m_restartAfterStop = false;
    SimuInterface* simulation = gpTessInterface->simuInterface();
    if (simulation->isRunning() || simulation->isPausing()) {
        m_comparisonPendingStart = true;
        m_comparisonRoundIndex = 0;
        m_comparisonAlgorithmIndex = 0;
        m_comparisonResultRound = 0;
        m_comparisonVehicleSeed = -1;
        m_comparisonDestinationSeed = -1;
        publishComparisonState(QStringLiteral("正在结束当前单次仿真，随后开始自动对照"));
        simulation->stopSimu();
        return;
    }
    beginComparisonBatch();
}

void MySimulator::requestCancelComparison()
{
    if (!m_comparisonActive && !m_comparisonPendingStart) return;
    if (m_comparisonPendingStart && !m_comparisonActive) {
        m_comparisonPendingStart = false;
        finishComparisonBatch(QStringLiteral("自动对照启动已取消"));
        return;
    }
    m_comparisonCancelRequested = true;
    publishComparisonState(QStringLiteral("正在停止，当前子实验将先保存"));
    SimuInterface* simulation = gpTessInterface->simuInterface();
    if (simulation->isRunning() || simulation->isPausing()) simulation->stopSimu();
    else finishComparisonBatch(QStringLiteral("自动对照已停止；已完成的数据仍保留"));
}
