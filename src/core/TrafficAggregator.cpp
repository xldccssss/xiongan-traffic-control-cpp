#include "TrafficAggregator.h"

#include <QtMath>

#include "ILane.h"
#include "ivehicle.h"
#include "ivehicledriving.h"

TrafficAggregator::TrafficAggregator(const ProjectModel* model)
{
    setModel(model);
}

void TrafficAggregator::setModel(const ProjectModel* model)
{
    m_model = model;
    reset();
}

void TrafficAggregator::reset()
{
    m_stages.clear();
    if (m_model) {
        m_stages.resize(m_model->intersections().size());
        m_previousStageVehicleIds.resize(m_model->intersections().size());
        m_stageArrivalTimes.resize(m_model->intersections().size());
        m_previousDownstreamOccupancy.resize(m_model->intersections().size());
        for (int i = 0; i < m_model->intersections().size(); ++i) {
            m_stages[i].resize(m_model->intersections()[i].stages.size());
            m_previousStageVehicleIds[i].resize(m_model->intersections()[i].stages.size());
            m_stageArrivalTimes[i].resize(m_model->intersections()[i].stages.size());
            m_previousDownstreamOccupancy[i].fill(
                0.0, m_model->intersections()[i].stages.size());
        }
    }
    m_links.clear();
    m_lanes.clear();
    m_telemetry = NetworkTelemetry();
    m_previousSpeed.clear();
    m_previousEnergyTime.clear();
    m_lastGenerated = 0;
    m_tractionEnergyKWh = 0.0;
    m_auxiliaryEnergyKWh = 0.0;
    m_regeneratedEnergyKWh = 0.0;
    m_distanceKm = 0.0;
}

void TrafficAggregator::sample(const QList<IVehicle*>& vehicles,
                               double simulationTimeSec,
                               qint64 generatedVehicles,
                               double actualMultiple)
{
    if (!m_model) return;
    for (QVector<StageTraffic>& intersection : m_stages)
        for (StageTraffic& stage : intersection) stage = StageTraffic();
    m_links.clear();
    m_lanes.clear();

    QHash<qint64, double> linkSpeedSum;
    QHash<qint64, double> laneSpeedSum;
    QVector<QVector<double>> incomingSpeedSum(m_stages.size());
    QVector<QVector<QSet<qint64>>> currentStageVehicleIds(m_stages.size());
    for (int i = 0; i < m_stages.size(); ++i) {
        incomingSpeedSum[i].resize(m_stages[i].size());
        currentStageVehicleIds[i].resize(m_stages[i].size());
        for (int s = 0; s < m_stages[i].size(); ++s) {
            const StageDefinition& definition = m_model->intersections()[i].stages[s];
            for (qint64 laneId : definition.incomingLaneIds) {
                m_stages[i][s].queueByLane.insert(laneId, 0.0);
                m_stages[i][s].serviceableQueueByLane.insert(laneId, 0.0);
            }
            m_stages[i][s].previousDownstreamOccupancy =
                m_previousDownstreamOccupancy[i].value(s, 0.0);
        }
    }

    double fleetSpeedSum = 0.0;
    int fleetQueue = 0;
    const QJsonObject energy = m_model->scenario().value("energy").toObject();
    const double mass = energy.value("vehicleMassKg").toDouble(1650.0);
    const double rollingCoefficient = energy.value(
        "rollingResistanceCoefficient").toDouble(0.011);
    const double dragCoefficient = energy.value("dragCoefficient").toDouble(0.29);
    const double frontalArea = energy.value("frontalAreaM2").toDouble(2.30);
    const double airDensity = energy.value("airDensityKgM3").toDouble(1.225);
    const double drivetrainEfficiency = qMax(1e-6,
        energy.value("drivetrainEfficiency").toDouble(0.90));
    const double regenerationEfficiency = qMax(0.0,
        energy.value("regenerativeEfficiency").toDouble(0.55));
    const double auxiliaryPowerKw = qMax(0.0,
        energy.value("auxiliaryPowerKw").toDouble(0.70));
    const double maxTractionKw = qMax(0.0,
        energy.value("maximumTractionPowerKw").toDouble(160.0));
    const double maxRegenerationKw = qMax(0.0,
        energy.value("maximumRegenerationPowerKw").toDouble(70.0));
    const double minAcceleration = energy.value(
        "minimumAccelerationMps2").toDouble(-6.0);
    const double maxAcceleration = energy.value(
        "maximumAccelerationMps2").toDouble(4.0);
    const double maxIntegrationGap = qMax(0.0,
        energy.value("maximumIntegrationGapSec").toDouble(5.0));
    const double laneObservationRadius = qMax(180.0,
        m_model->scenario().value("cav").toObject()
            .value("laneBalancing").toObject()
            .value("controlRadiusBeforeStopLineM").toDouble(320.0));
    double instantTractionKw = 0.0;
    double instantAuxiliaryKw = 0.0;
    double instantRegenerationKw = 0.0;
    QSet<qint64> liveIds;
    for (IVehicle* vehicle : vehicles) {
        if (!vehicle) continue;
        const qint64 vehicleId = vehicle->id();
        liveIds.insert(vehicleId);
        const double speed = qMax(0.0, static_cast<double>(vehicle->currSpeed(Metric)));
        fleetSpeedSum += speed;
        if (speed < 0.5) ++fleetQueue;

        // Exact port of core/energy.py: acceleration is derived from adjacent
        // speed observations because some SDK builds report currAcce()==0.
        const bool hasPrevious = m_previousSpeed.contains(vehicleId)
            && m_previousEnergyTime.contains(vehicleId);
        const double previousSpeed = hasPrevious
            ? m_previousSpeed.value(vehicleId) : speed;
        double dt = hasPrevious
            ? simulationTimeSec - m_previousEnergyTime.value(vehicleId) : 0.0;
        double acceleration = dt > 1e-9 ? (speed - previousSpeed) / dt : 0.0;
        if (dt < 0.0 || dt > maxIntegrationGap) {
            dt = 0.0;
            acceleration = 0.0;
        }
        acceleration = qBound(minAcceleration, acceleration, maxAcceleration);
        const double rollingForce = speed > 0.05
            ? mass * 9.80665 * rollingCoefficient : 0.0;
        const double aeroForce = 0.5 * airDensity * dragCoefficient
            * frontalArea * speed * speed;
        const double wheelPowerKw = (rollingForce + aeroForce
            + mass * acceleration) * speed / 1000.0;
        const double tractionKw = wheelPowerKw >= 0.0
            ? qMin(maxTractionKw, wheelPowerKw / drivetrainEfficiency) : 0.0;
        const double regenerationKw = wheelPowerKw < 0.0
            ? qMin(maxRegenerationKw, -wheelPowerKw * regenerationEfficiency) : 0.0;
        instantTractionKw += tractionKw;
        instantAuxiliaryKw += auxiliaryPowerKw;
        instantRegenerationKw += regenerationKw;
        if (dt > 0.0) {
            const double hours = dt / 3600.0;
            m_distanceKm += qMax(0.0,
                0.5 * (previousSpeed + speed) * dt / 1000.0);
            m_tractionEnergyKWh += tractionKw * hours;
            m_auxiliaryEnergyKWh += auxiliaryPowerKw * hours;
            m_regeneratedEnergyKWh += regenerationKw * hours;
        }
        m_previousSpeed.insert(vehicleId, speed);
        m_previousEnergyTime.insert(vehicleId, simulationTimeSec);

        if (!vehicle->roadIsLink()) continue;
        const qint64 linkId = vehicle->roadId();
        const qint64 laneId = vehicle->laneId();
        LinkTraffic& link = m_links[linkId];
        ++link.vehicleCount;
        if (speed < 0.5) ++link.queueCount;
        linkSpeedSum[linkId] += speed;

        double distanceToEnd = 1e9;
        if (vehicle->vehicleDriving())
            distanceToEnd = vehicle->vehicleDriving()->distToEndpoint(true, true, Metric);
        if (m_model->laneLinkById().contains(laneId)
            && distanceToEnd <= laneObservationRadius) {
            LaneTraffic& lane = m_lanes[laneId];
            ++lane.vehicleCount;
            const double laneQueueSpeed = m_model->scenario().value("cav").toObject()
                .value("laneBalancing").toObject()
                .value("queueSpeedThresholdMps").toDouble(2.0);
            if (speed < laneQueueSpeed) ++lane.queueCount;
            laneSpeedSum[laneId] += speed;
        }
        const QVector<StageRef> incomingRefs = m_model->incomingStagesByLane().value(laneId);
        if (distanceToEnd <= 180.0) {
            for (const StageRef& ref : incomingRefs) {
                StageTraffic& stage = m_stages[ref.intersection][ref.stage];
                ++stage.vehicleCount;
                currentStageVehicleIds[ref.intersection][ref.stage].insert(vehicleId);
                if (speed < 0.5) {
                    ++stage.queueVeh;
                    stage.queueByLane[laneId] += 1.0;
                    // The Python controller treats a physical shared lane as an
                    // atomic service unit for every compatible stage.
                    stage.serviceableQueueByLane[laneId] += 1.0;
                }
                incomingSpeedSum[ref.intersection][ref.stage] += speed;
            }
        }
        const QVector<StageRef> downstreamRefs = m_model->downstreamStagesByLane().value(laneId);
        double distanceFromStart = 1e9;
        if (vehicle->vehicleDriving())
            distanceFromStart = vehicle->vehicleDriving()->distToStartPoint(true, true, Metric);
        for (const StageRef& ref : downstreamRefs) {
            if (distanceFromStart > 180.0) continue;
            StageTraffic& stage = m_stages[ref.intersection][ref.stage];
            if (speed < 0.5) ++stage.downstreamQueueVeh;
            stage.downstreamOccupancy += 1.0;
        }
    }

    for (auto it = m_links.begin(); it != m_links.end(); ++it) {
        LinkTraffic& state = it.value();
        state.meanSpeedMps = state.vehicleCount > 0
            ? linkSpeedSum.value(it.key()) / state.vehicleCount : 0.0;
        const GraphLink graphLink = m_model->links().value(it.key());
        const double storage = qMax(7.5, graphLink.lengthM * qMax(1, graphLink.laneCount));
        state.occupancy = qBound(0.0, state.vehicleCount * 7.5 / storage, 1.0);
    }
    const double jamSpacing = qMax(1.0, m_model->scenario().value("cav")
        .toObject().value("jamSpacingM").toDouble(7.5));
    for (auto it = m_lanes.begin(); it != m_lanes.end(); ++it) {
        LaneTraffic& lane = it.value();
        lane.meanSpeedMps = lane.vehicleCount > 0
            ? laneSpeedSum.value(it.key()) / lane.vehicleCount : 13.9;
        const qint64 linkId = m_model->laneLinkById().value(it.key());
        const double observedLength = qMin(laneObservationRadius,
            qMax(0.1, m_model->links().value(linkId).lengthM));
        lane.occupancy = qBound(0.0,
            lane.vehicleCount / qMax(1.0, observedLength / jamSpacing), 1.0);
    }
    for (int i = 0; i < m_stages.size(); ++i) {
        for (int s = 0; s < m_stages[i].size(); ++s) {
            StageTraffic& stage = m_stages[i][s];
            QQueue<double>& arrivals = m_stageArrivalTimes[i][s];
            const QSet<qint64> newlyArrived = currentStageVehicleIds[i][s]
                - m_previousStageVehicleIds[i][s];
            for (qint64 unusedId : newlyArrived) {
                Q_UNUSED(unusedId);
                arrivals.enqueue(simulationTimeSec);
            }
            while (!arrivals.isEmpty() && arrivals.head() < simulationTimeSec - 60.0)
                arrivals.dequeue();
            m_previousStageVehicleIds[i][s] = currentStageVehicleIds[i][s];
            stage.arrivalsVehPerMin = arrivals.size();
            stage.meanSpeedMps = stage.vehicleCount > 0.0
                ? incomingSpeedSum[i][s] / stage.vehicleCount : 0.0;
            const StageDefinition& definition = m_model->intersections()[i].stages[s];
            stage.occupancy = qBound(
                0.0, stage.vehicleCount /
                    (qMax(1, definition.incomingLaneIds.size()) * 18.0), 1.0);
            stage.downstreamOccupancy = qBound(
                0.0, stage.downstreamOccupancy /
                    (qMax(1, definition.downstreamLaneIds.size()) * 18.0), 1.0);
            m_previousDownstreamOccupancy[i][s] = stage.downstreamOccupancy;
        }
    }

    m_telemetry.simulationTimeSec = simulationTimeSec;
    m_telemetry.runningVehicles = vehicles.size();
    m_telemetry.generatedVehicles = generatedVehicles;
    // generated-running is a removal count, not an OD completion count.  It
    // includes wrong exits, abnormal disappearances and preload effects.
    m_telemetry.removedVehicles = qMax<qint64>(0,
        generatedVehicles - vehicles.size());
    m_telemetry.completedVehicles = 0;
    m_telemetry.meanSpeedKph = vehicles.isEmpty() ? 0.0
        : fleetSpeedSum * 3.6 / vehicles.size();
    m_telemetry.queuedVehicles = fleetQueue;
    m_telemetry.totalQueueVeh = fleetQueue;
    m_telemetry.tractionEnergyKWh = m_tractionEnergyKWh;
    m_telemetry.auxiliaryEnergyKWh = m_auxiliaryEnergyKWh;
    m_telemetry.regeneratedEnergyKWh = m_regeneratedEnergyKWh;
    m_telemetry.energyKWh = qMax(0.0, m_tractionEnergyKWh
        + m_auxiliaryEnergyKWh - m_regeneratedEnergyKWh);
    m_telemetry.distanceKm = m_distanceKm;
    m_telemetry.estimatedCarbonKg = m_telemetry.energyKWh
        * energy.value("gridCarbonKgPerKwh").toDouble(0.5703);
    m_telemetry.instantTractionPowerKw = instantTractionKw;
    m_telemetry.instantAuxiliaryPowerKw = instantAuxiliaryKw;
    m_telemetry.instantRegenerationPowerKw = instantRegenerationKw;
    m_telemetry.instantNetPowerKw = instantTractionKw + instantAuxiliaryKw
        - instantRegenerationKw;
    m_telemetry.actualMultiple = actualMultiple;
    m_lastGenerated = generatedVehicles;

    for (auto it = m_previousSpeed.begin(); it != m_previousSpeed.end();) {
        if (!liveIds.contains(it.key())) {
            m_previousEnergyTime.remove(it.key());
            it = m_previousSpeed.erase(it);
        } else ++it;
    }
}

void TrafficAggregator::setRuntimeDiagnostics(
    int wrongExitBlockedVehicles, int pendingGreenReleaseVehicles,
    int connectorStallVehicles, int routeMaintenanceFailureCount,
    qint64 stopLineLaneChangeDismissVehicles, qint64 greenReleaseImpulseCount,
    qint64 connectorDeadlockReleaseImpulseCount)
{
    m_telemetry.wrongExitBlockedVehicles = qMax(0, wrongExitBlockedVehicles);
    m_telemetry.pendingGreenReleaseVehicles = qMax(0, pendingGreenReleaseVehicles);
    m_telemetry.connectorStallVehicles = qMax(0, connectorStallVehicles);
    m_telemetry.routeMaintenanceFailureCount = qMax(0, routeMaintenanceFailureCount);
    m_telemetry.stopLineLaneChangeDismissVehicles =
        qMax<qint64>(0, stopLineLaneChangeDismissVehicles);
    m_telemetry.greenReleaseImpulseCount = qMax<qint64>(0, greenReleaseImpulseCount);
    m_telemetry.connectorDeadlockReleaseImpulseCount =
        qMax<qint64>(0, connectorDeadlockReleaseImpulseCount);
}
