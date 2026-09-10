#pragma once

#include <QHash>
#include <QQueue>
#include <QSet>
#include <QVector>

#include "ProjectModel.h"

class RoutingEngine;
class SimuInterface;

struct SpawnVehicleType
{
    int code = 1;
    double speedFactor = 1.0;
    QString color = "#3D8BFF";
};

class DemandEngine
{
public:
    explicit DemandEngine(const ProjectModel* model = nullptr);
    void setModel(const ProjectModel* model);
    void reset();
    void setOuterRate(int vehiclesPerHour) { m_outerRate = qMax(0, vehiclesPerHour); }
    void setResidentialRate(int vehiclesPerHour) { m_residentialRate = qMax(0, vehiclesPerHour); }
    void setRandomSeed(int seed);
    void setPreloadLimit(int count);
    int outerRate() const { return m_outerRate; }
    int residentialRate() const { return m_residentialRate; }
    void step(double simulationTimeSec, double deltaSec, SimuInterface* simulation,
              RoutingEngine* routing);

    int preloadCreated() const { return m_preloadCreated; }
    int preloadTarget() const;
    qint64 continuousCreated() const { return m_continuousCreated; }
    QVector<int> vehicleTypeCycleCodesForTest() const;

private:
    bool createPlacement(const InitialPlacement& placement,
                         SimuInterface* simulation, RoutingEngine* routing);
    bool createAtTerminal(int terminalIndex, SimuInterface* simulation,
                          RoutingEngine* routing);
    void buildVehicleTypeCycle();

    const ProjectModel* m_model = nullptr;
    int m_outerRate = 1000;
    int m_residentialRate = 100;
    int m_vehicleSeed = 20260822;
    int m_preloadLimit = 0;
    int m_preloadCreated = 0;
    qint64 m_continuousCreated = 0;
    QVector<double> m_terminalCredit;
    QVector<double> m_nextRetrySec;
    QVector<double> m_lastTerminalReleaseSec;
    QQueue<int> m_pendingPlacements;
    QVector<int> m_placementAttempts;
    QVector<SpawnVehicleType> m_vehicleTypeCycle;
    QHash<qint64, int> m_laneCursorByLink;
    QSet<qint64> m_knownVehicleIds;
    int m_terminalRotation = 0;
    int m_vehicleTypeCursor = 0;
    qint64 m_vehicleNameSequence = 0;
    double m_lastGenerationSec = -1.0;
    double m_nowSec = 0.0;
};
