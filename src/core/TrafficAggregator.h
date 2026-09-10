#pragma once

#include <QHash>
#include <QQueue>
#include <QSet>
#include <QVector>

#include "ProjectModel.h"

class IVehicle;

struct StageTraffic
{
    double queueVeh = 0.0;
    double vehicleCount = 0.0;
    double meanSpeedMps = 0.0;
    double arrivalsVehPerMin = 0.0;
    double downstreamQueueVeh = 0.0;
    double downstreamOccupancy = 0.0;
    double occupancy = 0.0;
    double previousDownstreamOccupancy = 0.0;
    QHash<qint64, double> queueByLane;
    QHash<qint64, double> serviceableQueueByLane;
    bool laneObservationComplete = true;
    double measurementReliability = 0.85;
};

struct LinkTraffic
{
    int vehicleCount = 0;
    int queueCount = 0;
    double meanSpeedMps = 0.0;
    double occupancy = 0.0;
};

struct LaneTraffic
{
    int vehicleCount = 0;
    int queueCount = 0;
    double meanSpeedMps = 0.0;
    double occupancy = 0.0;
};

struct NetworkTelemetry
{
    double simulationTimeSec = 0.0;
    int runningVehicles = 0;
    qint64 generatedVehicles = 0;
    qint64 completedVehicles = 0;
    qint64 removedVehicles = 0;
    double meanSpeedKph = 0.0;
    int queuedVehicles = 0;
    double totalQueueVeh = 0.0;
    double energyKWh = 0.0;
    double tractionEnergyKWh = 0.0;
    double auxiliaryEnergyKWh = 0.0;
    double regeneratedEnergyKWh = 0.0;
    double estimatedCarbonKg = 0.0;
    double instantTractionPowerKw = 0.0;
    double instantAuxiliaryPowerKw = 0.0;
    double instantRegenerationPowerKw = 0.0;
    double instantNetPowerKw = 0.0;
    double distanceKm = 0.0;
    double actualMultiple = 0.0;
    int wrongExitBlockedVehicles = 0;
    int pendingGreenReleaseVehicles = 0;
    int connectorStallVehicles = 0;
    int routeMaintenanceFailureCount = 0;
    qint64 stopLineLaneChangeDismissVehicles = 0;
    qint64 greenReleaseImpulseCount = 0;
    qint64 connectorDeadlockReleaseImpulseCount = 0;
};

class TrafficAggregator
{
public:
    explicit TrafficAggregator(const ProjectModel* model = nullptr);
    void setModel(const ProjectModel* model);
    void reset();
    void sample(const QList<IVehicle*>& vehicles, double simulationTimeSec,
                qint64 generatedVehicles, double actualMultiple);
    void setRuntimeDiagnostics(int wrongExitBlockedVehicles,
                               int pendingGreenReleaseVehicles,
                               int connectorStallVehicles,
                               int routeMaintenanceFailureCount,
                               qint64 stopLineLaneChangeDismissVehicles,
                               qint64 greenReleaseImpulseCount,
                               qint64 connectorDeadlockReleaseImpulseCount);

    const QVector<QVector<StageTraffic>>& stages() const { return m_stages; }
    const QHash<qint64, LinkTraffic>& links() const { return m_links; }
    const QHash<qint64, LaneTraffic>& lanes() const { return m_lanes; }
    const NetworkTelemetry& telemetry() const { return m_telemetry; }

private:
    const ProjectModel* m_model = nullptr;
    QVector<QVector<StageTraffic>> m_stages;
    QHash<qint64, LinkTraffic> m_links;
    QHash<qint64, LaneTraffic> m_lanes;
    NetworkTelemetry m_telemetry;
    QHash<qint64, double> m_previousSpeed;
    QHash<qint64, double> m_previousEnergyTime;
    QVector<QVector<QSet<qint64>>> m_previousStageVehicleIds;
    QVector<QVector<QQueue<double>>> m_stageArrivalTimes;
    QVector<QVector<double>> m_previousDownstreamOccupancy;
    qint64 m_lastGenerated = 0;
    double m_tractionEnergyKWh = 0.0;
    double m_auxiliaryEnergyKWh = 0.0;
    double m_regeneratedEnergyKWh = 0.0;
    double m_distanceKm = 0.0;
};
