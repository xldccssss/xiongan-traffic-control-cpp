#pragma once

#include <QHash>
#include <QMutex>
#include <QVector>

#include "ProjectModel.h"
#include "TrafficAggregator.h"

class IDecisionPoint;
class ILaneConnector;
class IRouting;
class IVehicle;
class SignalEngine;

struct VehicleOdState
{
    int originTerminal = -1;
    int destinationTerminal = -1;
    qint64 originEntryLinkId = 0;
    qint64 destinationExitLinkId = 0;
    bool isInitialVehicle = false;
    double departureSec = 0.0;
    qint64 lockedDecisionLinkId = 0;
    qint64 lockedNextLinkId = 0;
    QVector<qint64> lockedRouteLinkIds;
    qint64 lastDecisionContextLinkId = 0;
    double lastDecisionAtSec = -1e9;
    qint64 lastSyncFailureLinkId = 0;
    qint64 lastSyncFailureTargetLinkId = 0;
    double lastSyncFailureAtSec = -1e9;
    int routeChanges = 0;
};

class RoutingEngine
{
public:
    explicit RoutingEngine(const ProjectModel* model = nullptr);
    void setModel(const ProjectModel* model);
    void reset();
    void setDestinationSeed(int seed);
    void rebuildShortestPathForests(const TrafficAggregator& traffic,
                                    const SignalEngine* signalEngine = nullptr,
                                    double nowSec = 0.0);
    void setSimulationTime(double nowSec);

    bool registerVehicle(IVehicle* vehicle, int originTerminal = -1,
                         int destinationTerminal = -1,
                         qint64 fixedDestinationExitLinkId = 0,
                         bool advanceOriginOrdinal = true);
    bool primeVehicleRoute(IVehicle* vehicle, qint64 currentLinkId,
                           bool constrainToCurrentLane = false);
    bool ensureVehicleRoute(IVehicle* vehicle, qint64 currentLinkId);
    void unregisterVehicle(qint64 vehicleId);
    void applyDecision(IVehicle* vehicle, IDecisionPoint* decisionPoint,
                       IRouting* nativeRouting);
    QList<ILaneConnector*> filterLaneConnectors(
        IVehicle* vehicle, const QList<ILaneConnector*>& candidates);

    int vehicleCountWithOd() const;
    qint64 routeDecisionCount() const;
    qint64 routeChangeCount() const;
    qint64 nativeRouteSyncCount() const;
    qint64 routeFallbackCount() const;
    VehicleOdState vehicleOdState(qint64 vehicleId) const;
    QHash<qint64, VehicleOdState> vehicleOdSnapshot() const;
    int nextDestinationForOrigin(int originTerminal) const;
    double freeFlowTravelTimeSec(qint64 startLinkId,
                                 qint64 destinationLinkId) const;
    qint64 movementTargetLink(qint64 vehicleId, qint64 currentLinkId = 0) const;
    qint64 recoverReachableMovementTarget(
        qint64 vehicleId, qint64 currentLinkId,
        const QVector<qint64>& reachableTargets);
    bool synchronizeNativeRoute(IVehicle* vehicle, qint64 currentLinkId,
                                qint64 reachableTargetLinkId);
    double edgeCostFor(qint64 fromLinkId, qint64 toLinkId) const;
    qint64 nextHopFor(qint64 destinationLinkId, qint64 currentLinkId) const;
    QVector<qint64> plannedPathForTest(qint64 startLinkId,
                                       qint64 destinationLinkId) const;
    int deterministicDestinationForTest(int originTerminal, qint64 ordinal) const
    { return deterministicDestination(originTerminal, ordinal); }

private:
    int deterministicDestination(int originTerminal, qint64 ordinal) const;
    int seededIndex(quint32 seed, int modulo, const QStringList& identity) const;
    qint64 chooseNext(qint64 vehicleId, qint64 currentLinkId,
                      const QVector<qint64>& available);
    QVector<qint64> buildExecutablePath(
        qint64 currentLinkId, qint64 destinationLinkId,
        const QVector<qint64>& routePrefix) const;
    bool applyExecutableRouting(IVehicle* vehicle, qint64 currentLinkId,
                                const QVector<qint64>& routePrefix,
                                QVector<qint64>* appliedLinkIds = nullptr);
    qint64 nativeMovementTarget(IVehicle* vehicle,
                                qint64 currentLinkId) const;
    bool isForbiddenDestinationTarget(const VehicleOdState& od,
                                      qint64 targetLinkId) const;
    double linkCost(qint64 linkId, const QHash<qint64, LinkTraffic>& traffic) const;

    const ProjectModel* m_model = nullptr;
    QHash<qint64, VehicleOdState> m_vehicleOd;
    QHash<qint64, IDecisionPoint*> m_decisionPointByLink;
    QHash<qint64, QHash<qint64, qint64>> m_nextHopByDestination;
    QHash<qint64, QHash<qint64, double>> m_distanceByDestination;
    QHash<qint64, QHash<qint64, double>> m_freeFlowDistanceByDestination;
    QHash<qint64, QHash<qint64, double>> m_edgeCost;
    QHash<QString, IRouting*> m_executableRoutingByPath;
    QHash<QString, int> m_reservations;
    QVector<qint64> m_originDepartureOrdinal;
    QJsonObject m_config;
    quint32 m_destinationSeed = 20260823u;
    double m_nowSec = 0.0;
    qint64 m_routeDecisions = 0;
    qint64 m_routeChanges = 0;
    qint64 m_nativeRouteSyncs = 0;
    qint64 m_routeFallbacks = 0;
    // TESSNG can invoke candidateLaneConnectors from native worker threads
    // while the main simulation callback rebuilds route costs.  Recursive
    // locking is required because public route operations call shared helpers.
    mutable QMutex m_mutex{QMutex::Recursive};
};
