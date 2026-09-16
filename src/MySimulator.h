#pragma once

#include <QHash>
#include <QElapsedTimer>
#include <QMutex>
#include <QSet>
#include <QObject>
#include <QVariantMap>
#include <QVector>

#include "Plugin/customersimulator.h"
#include "core/DemandEngine.h"
#include "core/CooperativeLaneBalancer.h"
#include "core/MetricsRecorder.h"
#include "core/ProjectModel.h"
#include "core/RoutingEngine.h"
#include "core/SignalEngine.h"
#include "core/SimulationRestartGuard.h"
#include "core/TrafficAggregator.h"

class ILaneConnector;
class ILaneObject;
class IDecisionPoint;
class IRouting;
class ISignalLamp;
class IVehicle;

class MySimulator : public QObject, public CustomerSimulator
{
    Q_OBJECT
public:
    explicit MySimulator(ProjectModel* model, QObject* parent = nullptr);
    ~MySimulator() override = default;

    void beforeStart(bool& keepOn) override;
    void afterStart() override;
    void afterPause() override;
    void afterStop() override;
    void afterOneStep() override;
    void initVehicle(IVehicle* vehicle) override;
    bool calcLampColor(ISignalLamp* signalLamp) override;
    void afterDistriRouting(IVehicle* vehicle, IDecisionPoint* decisionPoint,
                            IRouting* routing) override;
    QList<ILaneConnector*> candidateLaneConnectors(
        IVehicle* vehicle, QList<ILaneConnector*> candidates) override;
    QList<int> calcLimitedLaneNumber(IVehicle* vehicle) override;
    bool reCalcDismissChangeLane(IVehicle* vehicle) override;
    bool calcChangeLaneSafeDist(IVehicle* vehicle, qreal& distance) override;
    bool calcChangeLaneSafeDist(IVehicle* vehicle, qreal& distance,
                                UnitOfMeasure* unit) override;
    void beforeToLeftFreely(IVehicle* vehicle, bool& keepOn) override;
    void beforeToRightFreely(IVehicle* vehicle, bool& keepOn) override;
    void leaveForNextLaneObj(IVehicle* vehicle, ILaneObject* current,
                             ILaneObject* next) override;

public slots:
    void requestStart();
    void requestPause();
    void requestReset();
    void requestStop();
    void setAlgorithm(const QString& algorithm);
    void setTargetMultiple(int multiple);
    void setOuterRate(int rate);
    void setResidentialRate(int rate);
    void setDurationSec(int seconds);
    void setRandomSeeds(int vehicleSeed, int destinationSeed);
    void setPreloadCount(int count);
    void setSelectedIntersection(int index);
    void requestComparison(int rounds);
    void requestCancelComparison();

signals:
    void telemetryUpdated(const QVariantMap& values);
    void eventGenerated(const QString& text);
    void viewResetRequested();
    void comparisonStateChanged(bool active,
                                int currentRound,
                                int totalRounds,
                                const QString& algorithm,
                                int vehicleSeed,
                                int destinationSeed,
                                int resultRound,
                                const QString& status);

private:
    bool shouldFreezeApproachLane(IVehicle* vehicle) const;
    void bindSignalLamps();
    QString actualSignalColorForLane(qint64 laneId) const;
    void verifySignalReadback(double simulationTimeSec);
    void writeSignalFrame();
    void executeVehicleSafety(const QList<IVehicle*>& vehicles);
    void sampleTrajectories(const QList<IVehicle*>& vehicles);
    void publishTelemetry();
    void updateClosedLoopSpeed(double simulationTimeSec);
    bool runtimeSmokeTestRequested() const;
    bool runtimeResetSmokeTestRequested() const;
    double runtimeSmokeTestDurationSec() const;
    int runtimeSmokeTargetMultiple() const;
    void finishRuntimeSmokeTest(bool success, const QString& error = QString());
    QString algorithmName() const;
    void beginComparisonBatch();
    void prepareComparisonRun();
    void scheduleComparisonStart();
    void finishComparisonBatch(const QString& status);
    bool writeComparisonRoundManifest(QString* error = nullptr) const;
    void publishComparisonState(const QString& status);
    void scheduleManualResetStart(int delayMs);
    void cancelManualResetStart();

    ProjectModel* m_model = nullptr;
    // Native simulation workers may report vehicle departures concurrently
    // with the end-of-step callback. Protect the non-thread-safe metric and
    // vehicle bookkeeping containers shared by those two callback paths.
    mutable QMutex m_nativeCallbackMutex{QMutex::Recursive};
    DemandEngine m_demand;
    RoutingEngine m_routing;
    CooperativeLaneBalancer m_laneBalancer;
    SignalEngine m_signals;
    TrafficAggregator m_traffic;
    MetricsRecorder m_metrics;
    QVector<QPair<ISignalLamp*, qint64>> m_signalLamps;
    QHash<qint64, QVector<ISignalLamp*>> m_signalLampsByLane;
    QHash<qint64, ISignalLamp*> m_signalLampById;
    QHash<qint64, QString> m_expectedSignalColorByLampId;
    QHash<qint64, double> m_signalColorStableSinceSec;
    QHash<qint64, QVector<QVariantMap>> m_trajectories;
    QHash<qint64, qint64> m_lastVehicleLinkIds;
    QSet<qint64> m_signalHardStoppedVehicleIds;
    QHash<qint64, double> m_greenStoppedSinceSec;
    QHash<qint64, double> m_greenReleaseLastImpulseSec;
    QHash<qint64, int> m_greenReleaseImpulseAttempts;
    QHash<qint64, QVariantMap> m_greenPendingDetails;
    QHash<qint64, QVariantMap> m_connectorStallDetails;
    QSet<qint64> m_connectorDeadlockRecoveryVehicleIds;
    QHash<qint64, double> m_connectorDeadlockLastImpulseSec;
    QHash<qint64, int> m_connectorDeadlockImpulseAttempts;
    // Serialize recovery impulses inside one logical junction. This breaks a
    // mutual-yield cycle without pushing two crossing vehicles at once.
    QHash<QString, double> m_connectorDeadlockLastIntersectionReleaseSec;
    // Keep connector selection stable near the stop line. Upstream selection
    // may be refreshed slowly, but the executable callback and the per-step
    // command must both expose the same single connector after commitment.
    QHash<qint64, qint64> m_lockedVehicleConnectorIds;
    QHash<qint64, qint64> m_lockedVehicleConnectorFromLinkIds;
    QHash<qint64, double> m_lockedVehicleConnectorSelectedAtSec;
    QHash<qint64, int> m_lockedVehicleConnectorReselections;
    QHash<qint64, double> m_lastLaneChangeStartSecByLink;
    QSet<qint64> m_permissiveRightYieldVehicleIds;
    QSet<qint64> m_wrongExitBlockedVehicleIds;
    QSet<qint64> m_stopLineLaneChangeDismissVehicleIds;
    QSet<QString> m_routeMaintenanceFailures;
    double m_lastStepSec = 0.0;
    double m_lastAggregateSec = -1e9;
    double m_lastRouteRebuildSec = -1e9;
    double m_lastTelemetrySec = -1e9;
    double m_lastSignalReadbackSec = -1e9;
    double m_lastSignalWarningSec = -1e9;
    double m_signalReadbackTimeSec = 0.0;
    qint64 m_telemetrySampleCount = 0;
    qint64 m_recoveredMovementCount = 0;
    qint64 m_greenReleaseImpulseCount = 0;
    qint64 m_connectorDeadlockReleaseImpulseCount = 0;
    double m_maximumGreenReleaseWaitSec = 0.0;
    int m_signalReadbackAbnormalIntersections = 0;
    int m_targetMultiple = 1;
    int m_schedulerMultiple = 1;
    int m_nativeThreadCount = 1;
    int m_durationSec = 3600;
    int m_requestedVehicleSeed = 20260822;
    int m_requestedDestinationSeed = 20260823;
    int m_requestedPreloadCount = 0;
    int m_selectedIntersection = 0;
    int m_trajectoryRetentionPoints = 30;
    int m_maximumDisplayedVehicles = 400;
    bool m_running = false;
    SimulationRestartGuard m_manualResetGuard;
    std::uint64_t m_scheduledManualResetGeneration = 0;
    bool m_runtimeSmokeCompleted = false;
    bool m_runtimeResetTransitionRequested = false;
    int m_runtimeSmokeRunCount = 0;
    bool m_comparisonActive = false;
    bool m_comparisonPendingStart = false;
    bool m_comparisonCancelRequested = false;
    int m_comparisonRoundsTotal = 0;
    int m_comparisonRoundIndex = 0;
    int m_comparisonAlgorithmIndex = 0;
    int m_comparisonResultRound = 0;
    int m_comparisonVehicleSeed = -1;
    int m_comparisonDestinationSeed = -1;
    int m_manualVehicleSeedBeforeComparison = 20260822;
    int m_manualDestinationSeedBeforeComparison = 20260823;
    ControlBranch m_manualBranchBeforeComparison = ControlBranch::ImprovedMaxPressure;
    QString m_comparisonBatchId;
    QElapsedTimer m_speedClock;
    QVector<QPair<double, double>> m_speedMeasurements;
    double m_actualMultiple = 0.0;
    double m_lastSpeedAdjustWallSec = -1e9;
    double m_speedIntegralError = 0.0;
    int m_underTargetCount = 0;
    int m_capacityLimitCount = 0;
    bool m_speedCapacityLimited = false;
    ControlBranch m_requestedBranch = ControlBranch::ImprovedMaxPressure;
};
