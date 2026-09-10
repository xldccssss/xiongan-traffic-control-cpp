#pragma once

#include <QHash>
#include <QJsonObject>
#include <QPair>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVector>

struct StageDefinition
{
    QString id;
    QVector<QString> approachDirections;
    QVector<qint64> incomingLinkIds;
    QVector<qint64> incomingLaneIds;
    QVector<qint64> downstreamLaneIds;
    QVector<QString> turnTypes;
    QVector<QString> downstreamIntersectionIds;
    QVector<QPair<qint64, qint64>> movementLinkPairs;
    QVector<QPair<qint64, qint64>> straightMovementLinkPairs;
    int concurrentApproaches = 1;
    int startOffsetSec = 0;
    int greenSec = 12;
    int yellowSec = 3;
    int allRedSec = 2;
    bool sharedLaneSafetyFallback = false;
    bool conflictFree = true;
    bool opposingPaired = false;
    bool permissiveOpposing = false;
};

struct IntersectionDefinition
{
    QString id;
    QString name;
    bool signalized = true;
    // Keep the physical controller/phases/lamps for display and readback, but
    // hold every movement green and exclude the junction from phase switching.
    bool permanentGreen = false;
    QVector<QString> mergedMemberIds;
    QString junctionTypeName;
    QString elevationLayerName;
    QPointF centerM;
    qint64 connectorAreaId = 0;
    int armCount = 0;
    int cycleSec = 80;
    int yellowSec = 3;
    int allRedSec = 2;
    QVector<StageDefinition> stages;
};

struct StageRef
{
    int intersection = -1;
    int stage = -1;
};

struct TerminalDefinition
{
    QString id;
    QString group;
    qint64 entryLinkId = 0;
    qint64 exitLinkId = 0;
};

struct GraphLink
{
    qint64 id = 0;
    double lengthM = 1.0;
    int laneCount = 1;
    double freeSpeedMps = 13.9;
};

struct GraphEdge
{
    qint64 from = 0;
    qint64 to = 0;
    double connectorLengthM = 0.0;
};

struct LocalRouteOption
{
    qint64 nextLinkId = 0;
    qint64 routingChoiceLinkId = 0;
};

struct LocalDecision
{
    qint64 decisionLinkId = 0;
    qint64 routingContextLinkId = 0;
    QVector<qint64> corridorLinkIds;
    QVector<LocalRouteOption> routes;
    bool relocatedBeforeLaneWidening = false;
};

struct InitialPlacement
{
    int placementId = 0;
    qint64 linkId = 0;
    int laneNumber = 0;
    double distanceM = 0.0;
    double speedMps = 8.0;
    int vehicleTypeCode = 1;
    QString vehicleTypeKey;
    QString vehicleTypeName;
    QString color;
    QString originTerminalId;
    QString destinationTerminalId;
    qint64 destinationExitLinkId = 0;
};

class ProjectModel
{
public:
    bool load(const QString& projectRoot, QString* error);

    const QJsonObject& scenario() const { return m_scenario; }
    const QVector<IntersectionDefinition>& intersections() const { return m_intersections; }
    const QVector<TerminalDefinition>& terminals() const { return m_terminals; }
    const QHash<qint64, GraphLink>& links() const { return m_links; }
    const QVector<GraphEdge>& edges() const { return m_edges; }
    const QVector<InitialPlacement>& placements() const { return m_placements; }
    const QHash<qint64, LocalDecision>& decisionsByLink() const { return m_decisionsByLink; }
    const QHash<qint64, qint64>& decisionOwnerByCorridorLink() const { return m_decisionOwnerByCorridorLink; }
    const QHash<qint64, QVector<StageRef>>& incomingStagesByLane() const { return m_incomingStagesByLane; }
    const QHash<qint64, QVector<StageRef>>& controlledIncomingStagesByLane() const { return m_controlledIncomingStagesByLane; }
    const QHash<qint64, QVector<StageRef>>& downstreamStagesByLane() const { return m_downstreamStagesByLane; }
    const QHash<qint64, QVector<StageRef>>& incomingStagesByLink() const { return m_incomingStagesByLink; }
    const QHash<qint64, QSet<QString>>& turnTypesByLane() const { return m_turnTypesByLane; }
    const QHash<qint64, QHash<qint64, QString>>& turnTypeByLaneTarget() const { return m_turnTypeByLaneTarget; }
    const QHash<qint64, QHash<qint64, QVector<StageRef>>>& movementStagesByLink() const { return m_movementStagesByLink; }
    const QHash<qint64, QHash<qint64, QSet<qint64>>>& movementLanesByLink() const { return m_movementLanesByLink; }
    const QHash<qint64, qint64>& laneLinkById() const { return m_laneLinkById; }
    const QHash<qint64, QSet<qint64>>& blockedSpawnLanesByLink() const { return m_blockedSpawnLanesByLink; }
    const QHash<qint64, int>& terminalIndexByEntryLink() const { return m_terminalIndexByEntryLink; }
    const QHash<qint64, int>& terminalIndexByExitLink() const { return m_terminalIndexByExitLink; }
    const QSet<qint64>& forbiddenExitLinkIds() const { return m_forbiddenExitLinkIds; }
    const QSet<qint64>& disabledConnectorIds() const { return m_disabledConnectorIds; }
    bool isMovementDisabled(qint64 fromLinkId, qint64 toLinkId) const;
    int expectedSignalLampCount() const { return m_expectedSignalLampCount; }
    QString root() const { return m_root; }

private:
    static bool readJson(const QString& path, QJsonObject* result, QString* error);
    bool loadScenario(QString* error);
    bool loadIntersections(QString* error);
    bool loadTerminalsAndGraph(QString* error);
    bool loadLocalRouting(QString* error);
    bool loadInitialTraffic(QString* error);

    QString m_root;
    QJsonObject m_scenario;
    QVector<IntersectionDefinition> m_intersections;
    QVector<TerminalDefinition> m_terminals;
    QHash<qint64, GraphLink> m_links;
    QVector<GraphEdge> m_edges;
    QVector<InitialPlacement> m_placements;
    QHash<qint64, LocalDecision> m_decisionsByLink;
    QHash<qint64, qint64> m_decisionOwnerByCorridorLink;
    QHash<qint64, QVector<StageRef>> m_incomingStagesByLane;
    QHash<qint64, QVector<StageRef>> m_controlledIncomingStagesByLane;
    QHash<qint64, QVector<StageRef>> m_downstreamStagesByLane;
    QHash<qint64, QVector<StageRef>> m_incomingStagesByLink;
    QHash<qint64, QSet<QString>> m_turnTypesByLane;
    QHash<qint64, QHash<qint64, QString>> m_turnTypeByLaneTarget;
    QHash<qint64, QHash<qint64, QVector<StageRef>>> m_movementStagesByLink;
    QHash<qint64, QHash<qint64, QSet<qint64>>> m_movementLanesByLink;
    QHash<qint64, qint64> m_laneLinkById;
    QHash<qint64, QSet<qint64>> m_blockedSpawnLanesByLink;
    QHash<qint64, int> m_terminalIndexByEntryLink;
    QHash<qint64, int> m_terminalIndexByExitLink;
    QSet<qint64> m_forbiddenExitLinkIds;
    QSet<qint64> m_disabledConnectorIds;
    QSet<QPair<qint64, qint64>> m_disabledMovementLinkPairs;
    int m_expectedSignalLampCount = 0;
};
