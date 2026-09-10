#pragma once

#include <QHash>
#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QString>
#include <QVector>

#include "TrafficAggregator.h"

class IVehicle;
class ProjectModel;
class RoutingEngine;
class SignalEngine;

struct VehicleMetricState
{
    qint64 vehicleId = 0;
    qint64 originLinkId = 0;
    qint64 destinationLinkId = 0;
    qint64 lastLinkId = 0;
    double departureSec = 0.0;
    double freeFlowSec = -1.0;
    double lastSampleSec = -1.0;
    double missingSinceSec = -1.0;
    double lastSpeedMps = 0.0;
    double distanceM = 0.0;
    double queueTimeSec = 0.0;
    int stopCount = 0;
    bool hasMoved = false;
    bool wasStopped = false;
    bool eligible = false;
};

struct IntersectionMetricDiagnostic
{
    double maximumQueueVeh = 0.0;
    double greenObservedSec = 0.0;
    double greenQueuedSec = 0.0;
    double greenLowSpeedQueueSec = 0.0;
    double spillbackSec = 0.0;
    qint64 sampledMovementPassages = 0;
    qint64 sampledGreenStagePassages = 0;
    qint64 lastStageServiceCount = 0;
};

class MetricsRecorder
{
public:
    explicit MetricsRecorder(const QString& projectRoot = QString(),
                             const ProjectModel* model = nullptr);
    void setProjectRoot(const QString& projectRoot) { m_root = projectRoot; }
    void setModel(const ProjectModel* model) { m_model = model; }
    void start(const QString& algorithm,
               int vehicleSeed = -1,
               int destinationSeed = -1,
               int requestedRound = 0,
               const QString& comparisonBatchId = QString(),
               int comparisonRoundIndex = 0,
               int comparisonRoundCount = 0);
    void append(const NetworkTelemetry& telemetry);
    void observe(const QList<IVehicle*>& vehicles,
                 const TrafficAggregator& traffic,
                 RoutingEngine& routing,
                 const SignalEngine& signalEngine,
                 double simulationTimeSec);
    void finishVehicle(qint64 vehicleId, double simulationTimeSec,
                       bool reachedDestination, qint64 lastLinkId);
    double meanCompletedDelaySec() const;
    int completedDelaySampleCount() const;
    int eligibleCompletedVehicleCount() const { return m_completedTrips.size(); }
    int wrongDestinationVehicleCount() const { return m_wrongDestinationCount; }
    int nonTerminalDisappearanceCount() const { return m_nonTerminalDisappearanceCount; }
    QJsonObject performanceSnapshot() const { return performance(); }
    QString finish(const RoutingEngine* routing = nullptr,
                   QString* error = nullptr);
    int nextComparisonRound() const;

private:
    int chooseRound(const QString& algorithm) const;
    QJsonObject performance() const;
    static double percentile(QVector<double> values, double percent);

    QString m_root;
    QString m_algorithm;
    QString m_comparisonBatchId;
    const ProjectModel* m_model = nullptr;
    int m_vehicleSeed = -1;
    int m_destinationSeed = -1;
    int m_requestedRound = 0;
    int m_comparisonRoundIndex = 0;
    int m_comparisonRoundCount = 0;
    QVector<NetworkTelemetry> m_samples;
    QHash<qint64, VehicleMetricState> m_vehicleStates;
    QVector<QJsonObject> m_completedTrips;
    QVector<QJsonObject> m_abnormalTrips;
    QSet<qint64> m_registeredVehicleIds;
    int m_activeVehiclesAtSnapshot = 0;
    double m_officialStartSec = 15.0;
    double m_lastNetworkSec = -1.0;
    double m_observedUntilSec = 0.0;
    double m_previousQueue = 0.0;
    double m_previousSpillback = 0.0;
    double m_previousMeanImbalance = 0.0;
    double m_previousImbalanceExposure = 0.0;
    double m_previousAllLanesBusy = 0.0;
    double m_previousGreenStages = 0.0;
    double m_previousCombinedStages = 0.0;
    double m_previousFallbackStages = 0.0;
    double m_previousServedApproaches = 0.0;
    double m_queueVehicleSeconds = 0.0;
    double m_spillbackApproachSeconds = 0.0;
    double m_laneImbalanceSeconds = 0.0;
    double m_laneImbalanceExposureSeconds = 0.0;
    double m_allLanesCongestedSeconds = 0.0;
    double m_greenStageSeconds = 0.0;
    double m_combinedStageGreenSeconds = 0.0;
    double m_protectedFallbackGreenSeconds = 0.0;
    double m_parallelApproachServiceSeconds = 0.0;
    double m_concurrentServiceGainSeconds = 0.0;
    QVector<double> m_queueSamples;
    QVector<double> m_spillbackSamples;
    QVector<double> m_laneImbalanceSamples;
    QVector<double> m_maxLaneImbalanceSamples;
    QVector<IntersectionMetricDiagnostic> m_intersectionDiagnostics;
    int m_eligibleRegistered = 0;
    int m_eligibleAbnormal = 0;
    int m_initialCompleted = 0;
    int m_initialAbnormal = 0;
    int m_wrongDestinationCount = 0;
    int m_nonTerminalDisappearanceCount = 0;
};
