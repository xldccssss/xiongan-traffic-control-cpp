#pragma once

#include <QHash>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QVector>

#include "ProjectModel.h"
#include "TrafficAggregator.h"

class NetInterface;

enum class LaneChangeDirection { None, Left, Right };

struct LaneBalanceAction
{
    LaneChangeDirection direction = LaneChangeDirection::None;
    bool mandatory = false;
    QString reason;
    qint64 fromLaneId = 0;
    qint64 targetLaneId = 0;
    qint64 currentLinkId = 0;
    qint64 movementTargetLinkId = 0;
    double issuedAtSec = 0.0;
    double holdUntilSec = 0.0;

    bool valid() const
    { return direction != LaneChangeDirection::None && targetLaneId > 0; }
};

class CooperativeLaneBalancer
{
public:
    explicit CooperativeLaneBalancer(const ProjectModel* model = nullptr);
    void setModel(const ProjectModel* model);
    bool configure(NetInterface* network);
    void reset();

    LaneBalanceAction choose(qint64 vehicleId, qint64 currentLinkId,
                             qint64 currentLaneId, qint64 targetLinkId,
                             double distanceToEndM, double nowSec,
                             const TrafficAggregator& traffic);
    void cancelVehicleCommand(qint64 vehicleId);
    bool isLaneChangeLocked(qint64 linkId) const
    { return m_lockedLinkIds.contains(linkId); }
    int requestCount() const { return m_requestCount; }
    int completedCount() const { return m_completedCount; }
    int wideningCorridorCount() const { return m_wideningCorridorCount; }
    int wideningUnresolvedCount() const { return m_wideningUnresolvedCount; }
    bool topologyPolicyReady() const { return m_topologyPolicyReady; }
    int topologyPolicyBuildCount() const { return m_topologyPolicyBuildCount; }

private:
    struct LanePosition { qint64 linkId = 0; int index = -1; };
    struct VehicleState {
        LaneBalanceAction command;
        bool hasCommand = false;
        double cooldownUntilSec = 0.0;
        qint64 currentLinkId = 0;
        int attemptsOnCurrentLink = 0;
    };
    struct LaneMetric {
        double vehicleCount = 0.0;
        double queue = 0.0;
        double speed = 0.0;
        double occupancy = 0.0;
        double score = 0.0;
    };

    LaneMetric metric(qint64 laneId, const TrafficAggregator& traffic) const;
    LaneBalanceAction directionTowardAllowed(
        qint64 currentLaneId, const QSet<qint64>& allowed) const;
    LaneBalanceAction queueBalance(
        qint64 currentLinkId, qint64 currentLaneId,
        const QSet<qint64>& allowed, const TrafficAggregator& traffic) const;
    void buildWideningPolicy(NetInterface* network);
    void buildLaneOrders(NetInterface* network);

    const ProjectModel* m_model = nullptr;
    QJsonObject m_config;
    QHash<qint64, QHash<qint64, QSet<qint64>>> m_lanesByMovement;
    QSet<qint64> m_lockedLinkIds;
    QHash<qint64, QVector<qint64>> m_laneOrderByLink;
    QHash<qint64, LanePosition> m_lanePosition;
    QHash<qint64, qint64> m_leftNeighbor;
    QHash<qint64, qint64> m_rightNeighbor;
    QHash<qint64, VehicleState> m_vehicleStates;
    int m_requestCount = 0;
    int m_completedCount = 0;
    int m_wideningCorridorCount = 0;
    int m_wideningUnresolvedCount = 0;
    bool m_topologyPolicyReady = false;
    int m_topologyPolicyBuildCount = 0;
};
