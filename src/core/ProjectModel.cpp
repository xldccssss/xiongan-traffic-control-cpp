#include "ProjectModel.h"

#include <algorithm>
#include <functional>
#include <limits>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace {

QVector<qint64> int64Array(const QJsonValue& value)
{
    QVector<qint64> result;
    for (const QJsonValue& item : value.toArray())
        result.push_back(static_cast<qint64>(item.toDouble()));
    return result;
}

QVector<QString> stringArray(const QJsonValue& value)
{
    QVector<QString> result;
    for (const QJsonValue& item : value.toArray())
        result.push_back(item.toString());
    return result;
}

QVector<QPair<qint64, qint64>> int64PairArray(const QJsonValue& value)
{
    QVector<QPair<qint64, qint64>> result;
    for (const QJsonValue& item : value.toArray()) {
        const QJsonArray pair = item.toArray();
        if (pair.size() < 2) continue;
        const qint64 from = static_cast<qint64>(pair.at(0).toDouble());
        const qint64 to = static_cast<qint64>(pair.at(1).toDouble());
        if (from > 0 && to > 0) result.push_back({from, to});
    }
    return result;
}

void appendUnique(QVector<StageRef>& refs, const StageRef& ref)
{
    for (const StageRef& old : refs)
        if (old.intersection == ref.intersection && old.stage == ref.stage)
            return;
    refs.push_back(ref);
}

template <typename T>
void appendUniqueValues(QVector<T>& target, const QVector<T>& source)
{
    for (const T& value : source)
        if (!target.contains(value)) target.push_back(value);
}

QString singleApproachDirection(const StageDefinition& stage)
{
    if (stage.concurrentApproaches != 1 || stage.approachDirections.size() != 1)
        return QString();
    const QString direction = stage.approachDirections.first().trimmed().toUpper();
    return direction == "N" || direction == "S"
        || direction == "E" || direction == "W" ? direction : QString();
}

QString oppositeDirection(const QString& direction)
{
    if (direction == "N") return "S";
    if (direction == "S") return "N";
    if (direction == "E") return "W";
    if (direction == "W") return "E";
    return QString();
}

QSet<QString> normalizedTurns(const StageDefinition& stage)
{
    QSet<QString> turns;
    for (const QString& turn : stage.turnTypes)
        turns.insert(turn.trimmed().toLower());
    return turns;
}

int opposingPairScore(const StageDefinition& first,
                      const StageDefinition& second)
{
    const QSet<QString> a = normalizedTurns(first);
    const QSet<QString> b = normalizedTurns(second);
    int score = a == b ? 100 : 0;
    if (first.sharedLaneSafetyFallback && second.sharedLaneSafetyFallback)
        score += 60;
    if (a.contains("straight") && b.contains("straight")) score += 30;
    if (a.contains("left") == b.contains("left")) score += 10;
    score -= qAbs(first.incomingLaneIds.size() - second.incomingLaneIds.size());
    return score;
}

bool unsafePermissiveOpposingPair(const StageDefinition& first,
                                  const StageDefinition& second,
                                  int armCount)
{
    const QSet<QString> firstTurns = normalizedTurns(first);
    const QSet<QString> secondTurns = normalizedTurns(second);
    const bool leftThroughConflict =
        (firstTurns.contains("left") && secondTurns.contains("straight"))
        || (secondTurns.contains("left") && firstTurns.contains("straight"));
    const bool uturnThroughConflict =
        (firstTurns.contains("uturn") && secondTurns.contains("straight"))
        || (secondTurns.contains("uturn") && firstTurns.contains("straight"));
    if (!leftThroughConflict && !uturnThroughConflict)
        return false;

    // At a three-arm junction the two geometrically opposing arms are not a
    // symmetric cross-road pair: their left/through connectors converge on the
    // remaining arm, while a U-turn can cross the opposing through movement.
    // Combining them made vehicles yield inside the connector (notably at
    // J251), leaving a turning vehicle sideways across several lanes. Keep
    // those approaches protected and serve them separately.
    // Four-arm junctions retain the established bidirectional permissive model.
    return armCount <= 3;
}

StageDefinition mergeOpposingPair(const StageDefinition& first,
                                  const StageDefinition& second)
{
    StageDefinition merged = first;
    merged.id = first.id + QStringLiteral("__") + second.id;
    appendUniqueValues(merged.approachDirections, second.approachDirections);
    appendUniqueValues(merged.incomingLinkIds, second.incomingLinkIds);
    appendUniqueValues(merged.incomingLaneIds, second.incomingLaneIds);
    appendUniqueValues(merged.downstreamLaneIds, second.downstreamLaneIds);
    appendUniqueValues(merged.turnTypes, second.turnTypes);
    appendUniqueValues(merged.downstreamIntersectionIds,
                       second.downstreamIntersectionIds);
    appendUniqueValues(merged.movementLinkPairs, second.movementLinkPairs);
    appendUniqueValues(merged.straightMovementLinkPairs,
                       second.straightMovementLinkPairs);
    merged.concurrentApproaches = qMax(
        2, qMax(first.concurrentApproaches + second.concurrentApproaches,
                merged.incomingLinkIds.size()));
    merged.greenSec = qMax(first.greenSec, second.greenSec);
    merged.yellowSec = qMax(first.yellowSec, second.yellowSec);
    merged.allRedSec = qMax(first.allRedSec, second.allRedSec);
    merged.sharedLaneSafetyFallback = first.sharedLaneSafetyFallback
        || second.sharedLaneSafetyFallback;
    merged.opposingPaired = true;

    // A circular green can release opposing physical approaches together.
    // When one side contains a left movement and the other contains a through
    // movement, the combination is permissive rather than conflict-free:
    // TESSNG's native connector conflict model supplies the normal yield rule.
    const QSet<QString> firstTurns = normalizedTurns(first);
    const QSet<QString> secondTurns = normalizedTurns(second);
    merged.permissiveOpposing =
        (firstTurns.contains("left") && secondTurns.contains("straight"))
        || (secondTurns.contains("left") && firstTurns.contains("straight"));
    merged.conflictFree = first.conflictFree && second.conflictFree
        && !merged.permissiveOpposing;
    return merged;
}

void pairOpposingPhysicalApproaches(IntersectionDefinition& intersection)
{
    const QVector<StageDefinition> source = intersection.stages;
    QVector<bool> consumed(source.size(), false);
    QVector<StageDefinition> paired;
    paired.reserve(source.size());
    for (int i = 0; i < source.size(); ++i) {
        if (consumed[i]) continue;
        const QString direction = singleApproachDirection(source[i]);
        const QString wanted = oppositeDirection(direction);
        int best = -1;
        int bestScore = std::numeric_limits<int>::min();
        if (!wanted.isEmpty()) {
            for (int j = i + 1; j < source.size(); ++j) {
                if (consumed[j]
                    || singleApproachDirection(source[j]) != wanted)
                    continue;
                if (unsafePermissiveOpposingPair(source[i], source[j],
                                                 intersection.armCount))
                    continue;
                const int score = opposingPairScore(source[i], source[j]);
                if (best < 0 || score > bestScore) {
                    best = j;
                    bestScore = score;
                }
            }
        }
        if (best >= 0) {
            paired.push_back(mergeOpposingPair(source[i], source[best]));
            consumed[best] = true;
        } else {
            paired.push_back(source[i]);
        }
        consumed[i] = true;
    }

    int offsetSec = 0;
    for (StageDefinition& stage : paired) {
        stage.startOffsetSec = offsetSec;
        offsetSec += stage.greenSec + stage.yellowSec + stage.allRedSec;
    }
    intersection.stages = paired;
    intersection.cycleSec = qMax(1, offsetSec);
}

} // namespace

bool ProjectModel::readJson(const QString& path, QJsonObject* result, QString* error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("无法打开文件：%1").arg(QDir::toNativeSeparators(path));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error) *error = QStringLiteral("JSON解析失败：%1（%2）").arg(path, parseError.errorString());
        return false;
    }
    *result = document.object();
    return true;
}

bool ProjectModel::load(const QString& projectRoot, QString* error)
{
    m_root = QDir(projectRoot).absolutePath();
    m_intersections.clear();
    m_terminals.clear();
    m_links.clear();
    m_edges.clear();
    m_placements.clear();
    m_decisionsByLink.clear();
    m_decisionOwnerByCorridorLink.clear();
    m_incomingStagesByLane.clear();
    m_controlledIncomingStagesByLane.clear();
    m_downstreamStagesByLane.clear();
    m_incomingStagesByLink.clear();
    m_turnTypesByLane.clear();
    m_turnTypeByLaneTarget.clear();
    m_movementStagesByLink.clear();
    m_movementLanesByLink.clear();
    m_laneLinkById.clear();
    m_blockedSpawnLanesByLink.clear();
    m_terminalIndexByEntryLink.clear();
    m_terminalIndexByExitLink.clear();
    m_forbiddenExitLinkIds.clear();
    m_disabledConnectorIds.clear();
    m_disabledMovementLinkPairs.clear();
    m_expectedSignalLampCount = 0;
    return loadScenario(error) && loadIntersections(error)
        && loadTerminalsAndGraph(error) && loadLocalRouting(error)
        && loadInitialTraffic(error);
}

bool ProjectModel::loadScenario(QString* error)
{
    if (!readJson(QDir(m_root).filePath("config/scenario.json"),
                  &m_scenario, error))
        return false;
    const QJsonArray disabled = m_scenario.value("network").toObject()
        .value("disabledMovements").toArray();
    for (const QJsonValue& value : disabled) {
        const QJsonObject movement = value.toObject();
        const qint64 connectorId = static_cast<qint64>(
            movement.value("connectorId").toDouble());
        const qint64 fromLinkId = static_cast<qint64>(
            movement.value("fromLinkId").toDouble());
        const qint64 toLinkId = static_cast<qint64>(
            movement.value("toLinkId").toDouble());
        if (connectorId > 0) m_disabledConnectorIds.insert(connectorId);
        if (fromLinkId > 0 && toLinkId > 0)
            m_disabledMovementLinkPairs.insert({fromLinkId, toLinkId});
    }
    return true;
}

bool ProjectModel::isMovementDisabled(qint64 fromLinkId,
                                      qint64 toLinkId) const
{
    return m_disabledMovementLinkPairs.contains({fromLinkId, toLinkId});
}

bool ProjectModel::loadIntersections(QString* error)
{
    QJsonObject root;
    if (!readJson(QDir(m_root).filePath("generated/intersection_manifest.json"), &root, error))
        return false;
    const QJsonArray source = root.value("intersections").toArray();
    m_intersections.reserve(source.size());
    for (int i = 0; i < source.size(); ++i) {
        const QJsonObject value = source.at(i).toObject();
        IntersectionDefinition intersection;
        intersection.id = value.value("intersectionId").toString();
        intersection.name = value.value("name").toString(intersection.id);
        intersection.signalized = value.value("signalized").toBool(true);
        intersection.junctionTypeName = value.value("junctionTypeName").toString();
        intersection.elevationLayerName = value.value("elevationLayerName").toString();
        const QJsonArray center = value.value("centerXY").toArray();
        if (center.size() >= 2)
            intersection.centerM = QPointF(center.at(0).toDouble(), center.at(1).toDouble());
        intersection.connectorAreaId = static_cast<qint64>(
            value.value("connectorAreaId").toDouble());
        intersection.armCount = value.value("armCount").toInt();
        intersection.cycleSec = value.value("cycleSec").toInt(80);
        intersection.yellowSec = value.value("yellowSec").toInt(3);
        intersection.allRedSec = value.value("allRedSec").toInt(2);
        const QJsonArray stages = value.value("stages").toArray();
        intersection.stages.reserve(stages.size());
        for (int s = 0; s < stages.size(); ++s) {
            const QJsonObject stageValue = stages.at(s).toObject();
            StageDefinition stage;
            stage.id = stageValue.value("stageId").toString();
            stage.approachDirections = stringArray(
                stageValue.value("approachDirections"));
            if (stage.approachDirections.isEmpty()) {
                const QString direction = stageValue.value(
                    "approachDirection").toString();
                if (!direction.isEmpty()) stage.approachDirections.push_back(direction);
            }
            stage.incomingLinkIds = int64Array(stageValue.value("incomingLinkIds"));
            stage.incomingLaneIds = int64Array(stageValue.value("incomingLaneIds"));
            // Rebuild movement-derived fields after applying configured
            // closures so a disabled connector cannot remain routable through
            // a stale turn type or downstream-lane entry.
            stage.downstreamLaneIds.clear();
            stage.turnTypes.clear();
            stage.downstreamIntersectionIds = stringArray(
                stageValue.value("downstreamIntersectionIds"));
            for (const QJsonValue& movementValue : stageValue.value("movements").toArray()) {
                const QJsonObject movement = movementValue.toObject();
                const qint64 fromLaneId = static_cast<qint64>(
                    movement.value("fromLaneId").toDouble());
                const qint64 toLaneId = static_cast<qint64>(
                    movement.value("toLaneId").toDouble());
                const qint64 connectorId = static_cast<qint64>(
                    movement.value("connectorId").toDouble());
                const qint64 fromLinkId = static_cast<qint64>(
                    movement.value("fromLinkId").toDouble());
                const qint64 toLinkId = static_cast<qint64>(
                    movement.value("toLinkId").toDouble());
                const QString turnType = movement.value("turnType").toString().toLower();
                if (m_disabledConnectorIds.contains(connectorId)
                    || isMovementDisabled(fromLinkId, toLinkId))
                    continue;
                if (toLaneId > 0 && !stage.downstreamLaneIds.contains(toLaneId))
                    stage.downstreamLaneIds.push_back(toLaneId);
                if (!turnType.isEmpty() && !stage.turnTypes.contains(turnType))
                    stage.turnTypes.push_back(turnType);
                if (fromLaneId > 0 && toLinkId > 0 && !turnType.isEmpty())
                    m_turnTypeByLaneTarget[fromLaneId].insert(toLinkId, turnType);
                if (fromLaneId > 0 && !turnType.isEmpty())
                    m_turnTypesByLane[fromLaneId].insert(turnType);
                if (fromLaneId > 0 && fromLinkId > 0) {
                    m_laneLinkById.insert(fromLaneId, fromLinkId);
                    if (toLinkId > 0)
                        m_movementLanesByLink[fromLinkId][toLinkId].insert(fromLaneId);
                }
                if (fromLinkId > 0 && toLinkId > 0) {
                    const QPair<qint64, qint64> pair(fromLinkId, toLinkId);
                    if (!stage.movementLinkPairs.contains(pair))
                        stage.movementLinkPairs.push_back(pair);
                    if (turnType == QStringLiteral("straight")
                        && !stage.straightMovementLinkPairs.contains(pair))
                        stage.straightMovementLinkPairs.push_back(pair);
                }
            }
            stage.concurrentApproaches = qMax(1, stageValue.value("concurrentApproachCount").toInt(1));
            stage.startOffsetSec = stageValue.value("startOffsetSec").toInt(0);
            stage.greenSec = stageValue.value("greenSec").toInt(12);
            stage.yellowSec = stageValue.value("yellowSec").toInt(intersection.yellowSec);
            stage.allRedSec = stageValue.value("allRedSec").toInt(intersection.allRedSec);
            stage.sharedLaneSafetyFallback = stageValue.value(
                "sharedLaneSafetyFallback").toBool(false);
            stage.conflictFree = stageValue.value("conflictFree").toBool(true);

            // signalLampBindings is the authoritative physical-lane atomic mapping.
            // It can contain shared left/straight/right movements on one lane.
            const QJsonArray bindings = stageValue.value("signalLampBindings").toArray();
            if (!bindings.isEmpty()) {
                stage.incomingLaneIds.clear();
                QSet<qint64> laneSet;
                for (const QJsonValue& bindingValue : bindings) {
                    const QJsonObject binding = bindingValue.toObject();
                    const qint64 laneId = static_cast<qint64>(
                        binding.value("fromLaneId").toDouble());
                    if (laneId > 0) {
                        laneSet.insert(laneId);
                        // Movement rows are more precise than the aggregate
                        // lamp binding after connector-level closures.  Retain
                        // the binding only as a fallback for malformed legacy
                        // manifests without movement rows.
                        if (!m_turnTypeByLaneTarget.contains(laneId))
                            for (const QJsonValue& turn
                                 : binding.value("turnTypes").toArray())
                                m_turnTypesByLane[laneId].insert(turn.toString());
                    }
                }
                for (qint64 laneId : laneSet) stage.incomingLaneIds.push_back(laneId);
            }
            intersection.stages.push_back(stage);
        }
        m_intersections.push_back(intersection);
    }

    // Close junctions can be represented by more than one ConnectorArea in
    // the source network. Merge them before building lane/stage indexes so
    // they become one logical signalized intersection with one outer stop-line
    // boundary, rather than two controllers only a few metres apart.
    QHash<QString, QString> mergedPrimaryByMember;
    const QJsonArray mergeGroups = m_scenario.value("signals").toObject()
        .value("mergedIntersections").toArray();
    auto findIntersectionIndex = [this](const QString& id) {
        for (int index = 0; index < m_intersections.size(); ++index)
            if (m_intersections[index].id == id) return index;
        return -1;
    };
    auto removeLaneMovementIndexes = [this](const QVector<qint64>& laneIds) {
        for (qint64 laneId : laneIds) {
            const qint64 fromLinkId = m_laneLinkById.value(laneId, 0);
            for (qint64 toLinkId : m_turnTypeByLaneTarget.value(laneId).keys()) {
                QSet<qint64>& lanes = m_movementLanesByLink[fromLinkId][toLinkId];
                lanes.remove(laneId);
                if (lanes.isEmpty()) m_movementLanesByLink[fromLinkId].remove(toLinkId);
            }
            if (m_movementLanesByLink.value(fromLinkId).isEmpty())
                m_movementLanesByLink.remove(fromLinkId);
            m_turnTypeByLaneTarget.remove(laneId);
            m_turnTypesByLane.remove(laneId);
            m_laneLinkById.remove(laneId);
        }
    };
    for (const QJsonValue& groupValue : mergeGroups) {
        const QJsonObject group = groupValue.toObject();
        const QString primaryId = group.value("primaryIntersectionId").toString();
        const int primaryIndex = findIntersectionIndex(primaryId);
        if (primaryIndex < 0) {
            if (error) *error = QStringLiteral("合并交叉口主节点不存在：%1").arg(primaryId);
            return false;
        }
        IntersectionDefinition merged = m_intersections[primaryIndex];
        merged.name = group.value("displayName").toString(merged.name);
        merged.junctionTypeName = group.value("junctionTypeName")
            .toString(merged.junctionTypeName);
        const QJsonArray mergedCenter = group.value("centerXY").toArray();
        if (mergedCenter.size() >= 2)
            merged.centerM = QPointF(mergedCenter.at(0).toDouble(),
                                     mergedCenter.at(1).toDouble());

        QVector<IntersectionDefinition> members;
        QVector<int> memberIndexes;
        for (const QString& memberId : stringArray(group.value("memberIntersectionIds"))) {
            const int memberIndex = findIntersectionIndex(memberId);
            if (memberIndex < 0 || memberIndex == primaryIndex) {
                if (error) *error = QStringLiteral("合并交叉口成员不存在：%1").arg(memberId);
                return false;
            }
            members.push_back(m_intersections[memberIndex]);
            memberIndexes.push_back(memberIndex);
            merged.mergedMemberIds.push_back(memberId);
            mergedPrimaryByMember.insert(memberId, primaryId);
        }

        for (const QJsonValue& replacementValue
             : group.value("stageApproachReplacements").toArray()) {
            const QJsonObject replacement = replacementValue.toObject();
            const QString stageId = replacement.value("stageId").toString();
            int stageIndex = -1;
            for (int index = 0; index < merged.stages.size(); ++index)
                if (merged.stages[index].id == stageId) { stageIndex = index; break; }
            if (stageIndex < 0) {
                if (error) *error = QStringLiteral("合并交叉口阶段不存在：%1/%2")
                    .arg(primaryId, stageId);
                return false;
            }
            StageDefinition& target = merged.stages[stageIndex];
            const QVector<qint64> oldIncomingLanes = target.incomingLaneIds;
            const QVector<qint64> removeLanes = int64Array(
                replacement.value("removeIncomingLaneIds"));
            const QVector<qint64> addLanes = int64Array(
                replacement.value("addIncomingLaneIds"));
            const QVector<qint64> removeLinks = int64Array(
                replacement.value("removeIncomingLinkIds"));
            const QVector<qint64> addLinks = int64Array(
                replacement.value("addIncomingLinkIds"));
            const QVector<qint64> removeDownstream = int64Array(
                replacement.value("removeDownstreamLaneIds"));
            const QVector<qint64> addDownstream = int64Array(
                replacement.value("addDownstreamLaneIds"));
            removeLaneMovementIndexes(oldIncomingLanes);
            if (replacement.contains("replaceIncomingLaneIds"))
                target.incomingLaneIds = int64Array(
                    replacement.value("replaceIncomingLaneIds"));
            else {
                for (qint64 id : removeLanes) target.incomingLaneIds.removeAll(id);
                for (qint64 id : addLanes)
                    if (!target.incomingLaneIds.contains(id)) target.incomingLaneIds.push_back(id);
            }
            if (replacement.contains("replaceIncomingLinkIds"))
                target.incomingLinkIds = int64Array(
                    replacement.value("replaceIncomingLinkIds"));
            else {
                for (qint64 id : removeLinks) target.incomingLinkIds.removeAll(id);
                for (qint64 id : addLinks)
                    if (!target.incomingLinkIds.contains(id)) target.incomingLinkIds.push_back(id);
            }
            if (replacement.contains("replaceDownstreamLaneIds"))
                target.downstreamLaneIds = int64Array(
                    replacement.value("replaceDownstreamLaneIds"));
            else {
                for (qint64 id : removeDownstream) target.downstreamLaneIds.removeAll(id);
                for (qint64 id : addDownstream)
                    if (!target.downstreamLaneIds.contains(id)) target.downstreamLaneIds.push_back(id);
            }
            for (int pairIndex = target.movementLinkPairs.size() - 1;
                 pairIndex >= 0; --pairIndex)
                if (removeLinks.contains(target.movementLinkPairs[pairIndex].first))
                    target.movementLinkPairs.remove(pairIndex);
            for (int pairIndex = target.straightMovementLinkPairs.size() - 1;
                 pairIndex >= 0; --pairIndex)
                if (removeLinks.contains(
                        target.straightMovementLinkPairs[pairIndex].first))
                    target.straightMovementLinkPairs.remove(pairIndex);

            if (replacement.contains("replaceMovementLinkPairs"))
                target.movementLinkPairs = int64PairArray(
                    replacement.value("replaceMovementLinkPairs"));
            if (replacement.contains("replaceStraightMovementLinkPairs"))
                target.straightMovementLinkPairs = int64PairArray(
                    replacement.value("replaceStraightMovementLinkPairs"));
            if (replacement.contains("replaceTurnTypes"))
                target.turnTypes = stringArray(
                    replacement.value("replaceTurnTypes"));

            const QString sourceStageId = replacement.value(
                "sourceMemberStageId").toString();
            for (const IntersectionDefinition& member : members) {
                for (const StageDefinition& sourceStage : member.stages) {
                    if (sourceStage.id != sourceStageId) continue;
                    for (const QPair<qint64, qint64>& movement
                         : sourceStage.movementLinkPairs)
                        if (addLinks.contains(movement.first)
                            && !target.movementLinkPairs.contains(movement))
                            target.movementLinkPairs.push_back(movement);
                    for (const QPair<qint64, qint64>& movement
                         : sourceStage.straightMovementLinkPairs)
                        if (addLinks.contains(movement.first)
                            && !target.straightMovementLinkPairs.contains(movement))
                            target.straightMovementLinkPairs.push_back(movement);
                    for (const QString& turn : sourceStage.turnTypes)
                        if (!target.turnTypes.contains(turn)) target.turnTypes.push_back(turn);
                }
            }

            // A hand-edited TESS network can replace the links and lane IDs
            // at a merged junction while the generated manifest remains the
            // reproducible source snapshot.  Rebuild the lane-level movement
            // indexes from this small explicit override so signal guards,
            // route recovery and lane balancing all see the real topology.
            for (const QJsonValue& movementValue
                 : replacement.value("laneMovements").toArray()) {
                const QJsonObject movement = movementValue.toObject();
                const qint64 fromLaneId = static_cast<qint64>(
                    movement.value("fromLaneId").toDouble());
                const qint64 fromLinkId = static_cast<qint64>(
                    movement.value("fromLinkId").toDouble());
                const qint64 toLinkId = static_cast<qint64>(
                    movement.value("toLinkId").toDouble());
                const QString turnType = movement.value("turnType")
                    .toString().trimmed().toLower();
                if (fromLaneId <= 0 || fromLinkId <= 0 || toLinkId <= 0
                    || turnType.isEmpty())
                    continue;
                m_laneLinkById.insert(fromLaneId, fromLinkId);
                m_turnTypeByLaneTarget[fromLaneId].insert(toLinkId, turnType);
                m_turnTypesByLane[fromLaneId].insert(turnType);
                m_movementLanesByLink[fromLinkId][toLinkId].insert(fromLaneId);
            }
        }


        for (const IntersectionDefinition& member : members)
            for (const StageDefinition& stage : member.stages)
                removeLaneMovementIndexes(stage.incomingLaneIds);

        std::sort(memberIndexes.begin(), memberIndexes.end(), std::greater<int>());
        for (int memberIndex : memberIndexes) m_intersections.remove(memberIndex);
        const int updatedPrimaryIndex = findIntersectionIndex(primaryId);
        if (updatedPrimaryIndex < 0) {
            if (error) *error = QStringLiteral("合并后主交叉口丢失：%1").arg(primaryId);
            return false;
        }
        m_intersections[updatedPrimaryIndex] = merged;
    }

    for (IntersectionDefinition& intersection : m_intersections) {
        for (StageDefinition& stage : intersection.stages) {
            for (QString& downstreamId : stage.downstreamIntersectionIds)
                if (mergedPrimaryByMember.contains(downstreamId))
                    downstreamId = mergedPrimaryByMember.value(downstreamId);
            stage.downstreamIntersectionIds.removeAll(intersection.id);
        }
    }

    const QSet<QString> permanentGreenIds = [&]() {
        QSet<QString> ids;
        for (const QString& id : stringArray(m_scenario.value("signals")
                 .toObject().value("permanentGreenIntersectionIds")))
            ids.insert(mergedPrimaryByMember.value(id, id));
        return ids;
    }();
    for (IntersectionDefinition& intersection : m_intersections)
        intersection.permanentGreen = permanentGreenIds.contains(intersection.id);

    // Most real four-arm junctions use two circular-green stages: N/S and
    // E/W.  The source conflict graph deliberately split any shared
    // left/through lane into single-approach protected stages.  That is safe
    // but wastes roughly half the junction capacity.  Pair opposite physical
    // approaches here; conflicting left turns remain permissive and yield via
    // TESSNG's native connector model.  Cross-axis movements are never paired.
    const bool pairOpposing = m_scenario.value("signals").toObject()
        .value("pairOpposingPhysicalApproaches").toBool(true);
    if (pairOpposing)
        for (IntersectionDefinition& intersection : m_intersections)
            if (intersection.signalized && !intersection.permanentGreen)
                pairOpposingPhysicalApproaches(intersection);

    m_expectedSignalLampCount = 0;
    for (const IntersectionDefinition& intersection : m_intersections)
        if (intersection.signalized)
            for (const StageDefinition& stage : intersection.stages)
                m_expectedSignalLampCount += stage.incomingLaneIds.size();

    for (int i = 0; i < m_intersections.size(); ++i) {
        for (int s = 0; s < m_intersections[i].stages.size(); ++s) {
            const StageRef ref{i, s};
            const StageDefinition& stage = m_intersections[i].stages[s];
            for (qint64 laneId : stage.incomingLaneIds) {
                appendUnique(m_incomingStagesByLane[laneId], ref);
                if (m_intersections[i].signalized)
                    appendUnique(m_controlledIncomingStagesByLane[laneId], ref);
            }
            for (qint64 laneId : stage.downstreamLaneIds)
                appendUnique(m_downstreamStagesByLane[laneId], ref);
            for (qint64 linkId : stage.incomingLinkIds)
                appendUnique(m_incomingStagesByLink[linkId], ref);
            for (const QPair<qint64, qint64>& movement : stage.movementLinkPairs)
                appendUnique(m_movementStagesByLink[movement.first][movement.second], ref);
        }
    }
    if (m_intersections.isEmpty()) {
        if (error) *error = QStringLiteral("交叉口清单为空");
        return false;
    }
    QJsonObject audit;
    QString ignored;
    if (readJson(QDir(m_root).filePath("generated/signal_structure_audit.json"),
                 &audit, &ignored)) {
        for (const QJsonValue& warningValue : audit.value("warnings").toArray()) {
            const QJsonObject warning = warningValue.toObject();
            if (warning.value("code").toString() != "APPROACH_LANE_WITHOUT_EXIT")
                continue;
            const qint64 linkId = static_cast<qint64>(warning.value("linkId").toDouble());
            const qint64 laneId = static_cast<qint64>(warning.value("laneId").toDouble());
            if (linkId > 0 && laneId > 0) m_blockedSpawnLanesByLink[linkId].insert(laneId);
        }
    }
    const QJsonObject topologyOverrides = m_scenario.value("network")
        .toObject().value("topologyOverrides").toObject();
    for (const QJsonValue& value
         : topologyOverrides.value("blockedSpawnLanes").toArray()) {
        const QJsonObject item = value.toObject();
        const qint64 linkId = static_cast<qint64>(item.value("linkId").toDouble());
        const qint64 laneId = static_cast<qint64>(item.value("laneId").toDouble());
        if (linkId > 0 && laneId > 0)
            m_blockedSpawnLanesByLink[linkId].insert(laneId);
    }
    return true;
}

bool ProjectModel::loadTerminalsAndGraph(QString* error)
{
    QJsonObject root;
    if (!readJson(QDir(m_root).filePath("generated/dynamic_routing_graph.json"), &root, error))
        return false;
    const QJsonObject topologyOverrides = m_scenario.value("network")
        .toObject().value("topologyOverrides").toObject();
    QSet<qint64> removedLinkIds;
    for (qint64 linkId : int64Array(topologyOverrides.value("removedLinkIds")))
        removedLinkIds.insert(linkId);
    for (const QJsonValue& value : root.value("links").toArray()) {
        const QJsonObject item = value.toObject();
        GraphLink link;
        link.id = static_cast<qint64>(item.value("linkId").toDouble());
        link.lengthM = qMax(1.0, item.value("lengthM").toDouble(1.0));
        link.laneCount = qMax(1, item.value("laneCount").toInt(1));
        const double desiredKph = item.value("desiredSpeedKph").toDouble(50.0);
        const double limitKph = item.value("limitSpeedKph").toDouble(desiredKph);
        link.freeSpeedMps = qMax(1.0, qMin(
            desiredKph > 0.0 ? desiredKph : 50.0,
            limitKph > 0.0 ? limitKph : desiredKph) / 3.6);
        if (!removedLinkIds.contains(link.id)) m_links.insert(link.id, link);
    }
    for (const QJsonValue& value : root.value("edges").toArray()) {
        const QJsonObject item = value.toObject();
        GraphEdge edge;
        edge.from = static_cast<qint64>(item.value("fromLinkId").toDouble());
        edge.to = static_cast<qint64>(item.value("toLinkId").toDouble());
        edge.connectorLengthM = item.value("connectorLengthM").toDouble(0.0);
        if (edge.from > 0 && edge.to > 0
            && !removedLinkIds.contains(edge.from)
            && !removedLinkIds.contains(edge.to)
            && !isMovementDisabled(edge.from, edge.to))
            m_edges.push_back(edge);
    }
    for (const QJsonValue& value
         : topologyOverrides.value("addedEdges").toArray()) {
        const QJsonObject item = value.toObject();
        GraphEdge edge;
        edge.from = static_cast<qint64>(item.value("fromLinkId").toDouble());
        edge.to = static_cast<qint64>(item.value("toLinkId").toDouble());
        edge.connectorLengthM = qMax(
            0.0, item.value("connectorLengthM").toDouble(0.0));
        if (edge.from <= 0 || edge.to <= 0
            || !m_links.contains(edge.from) || !m_links.contains(edge.to)
            || isMovementDisabled(edge.from, edge.to))
            continue;
        for (int index = m_edges.size() - 1; index >= 0; --index)
            if (m_edges[index].from == edge.from
                && m_edges[index].to == edge.to)
                m_edges.remove(index);
        m_edges.push_back(edge);
    }
    int index = 0;
    for (const QJsonValue& value : root.value("terminals").toArray()) {
        const QJsonObject item = value.toObject();
        TerminalDefinition terminal;
        terminal.id = item.value("terminalId").toString();
        terminal.group = terminal.id.startsWith('R') ? "RESIDENTIAL_AREA" : "OUTER_BOUNDARY";
        terminal.entryLinkId = static_cast<qint64>(item.value("entryLinkId").toDouble());
        terminal.exitLinkId = static_cast<qint64>(item.value("exitLinkId").toDouble());
        m_terminalIndexByEntryLink.insert(terminal.entryLinkId, index);
        m_terminalIndexByExitLink.insert(terminal.exitLinkId, index);
        m_terminals.push_back(terminal);
        ++index;
    }
    if (m_terminals.size() != 22) {
        if (error) *error = QStringLiteral("期望22对端点，实际读取%1对").arg(m_terminals.size());
        return false;
    }
    // Every graph sink that is not one of the 22 configured OD exits is an
    // invalid escape from the experiment network.  The current network has
    // one such automatically detected boundary (link 5380).  Deriving this
    // set from the graph keeps the guard valid if the network is regenerated.
    QSet<qint64> linksWithOutgoingEdges;
    for (const GraphEdge& edge : m_edges) linksWithOutgoingEdges.insert(edge.from);
    for (auto it = m_links.cbegin(); it != m_links.cend(); ++it)
        if (!linksWithOutgoingEdges.contains(it.key())
            && !m_terminalIndexByExitLink.contains(it.key()))
            m_forbiddenExitLinkIds.insert(it.key());
    return true;
}

bool ProjectModel::loadLocalRouting(QString* error)
{
    QJsonObject root;
    if (!readJson(QDir(m_root).filePath("generated/local_routing_manifest.json"), &root, error))
        return false;
    const QJsonObject topologyOverrides = m_scenario.value("network")
        .toObject().value("topologyOverrides").toObject();
    QSet<qint64> removedLinkIds;
    for (qint64 linkId : int64Array(topologyOverrides.value("removedLinkIds")))
        removedLinkIds.insert(linkId);
    auto loadDecision = [&](const QJsonObject& item) {
        LocalDecision decision;
        decision.decisionLinkId = static_cast<qint64>(item.value("decisionLinkId").toDouble());
        decision.routingContextLinkId = static_cast<qint64>(
            item.value("intersectionIncomingLinkId").toDouble(decision.decisionLinkId));
        decision.corridorLinkIds = int64Array(item.value("decisionCorridorLinkIds"));
        if (decision.corridorLinkIds.isEmpty())
            decision.corridorLinkIds = int64Array(item.value("corridorLinkIds"));
        if (decision.corridorLinkIds.isEmpty()) decision.corridorLinkIds.push_back(decision.decisionLinkId);
        decision.relocatedBeforeLaneWidening = item.value(
            "relocatedBeforeLaneWidening").toBool(false);
        for (const QJsonValue& routeValue : item.value("routes").toArray()) {
            const QJsonObject routeObject = routeValue.toObject();
            LocalRouteOption option;
            option.nextLinkId = static_cast<qint64>(routeObject.value("toLinkId").toDouble());
            option.routingChoiceLinkId = static_cast<qint64>(
                routeObject.value("routingChoiceLinkId").toDouble(option.nextLinkId));
            if (option.nextLinkId > 0
                && !removedLinkIds.contains(option.nextLinkId)
                && !isMovementDisabled(decision.routingContextLinkId,
                                       option.nextLinkId))
                decision.routes.push_back(option);
        }
        if (decision.decisionLinkId > 0 && !decision.routes.isEmpty()
            && !removedLinkIds.contains(decision.decisionLinkId)
            && !removedLinkIds.contains(decision.routingContextLinkId)) {
            QVector<qint64> physicalCorridor;
            for (qint64 linkId : decision.corridorLinkIds)
                if (!removedLinkIds.contains(linkId))
                    physicalCorridor.push_back(linkId);
            decision.corridorLinkIds = physicalCorridor;
            if (decision.corridorLinkIds.isEmpty())
                decision.corridorLinkIds.push_back(decision.decisionLinkId);
            m_decisionsByLink.insert(decision.decisionLinkId, decision);
            for (qint64 linkId : decision.corridorLinkIds)
                m_decisionOwnerByCorridorLink.insert(linkId, decision.decisionLinkId);
        }
    };
    for (const QJsonValue& value : root.value("decisions").toArray())
        loadDecision(value.toObject());
    for (const QJsonValue& value
         : topologyOverrides.value("localDecisionOverrides").toArray()) {
        const QJsonObject item = value.toObject();
        const qint64 decisionLinkId = static_cast<qint64>(
            item.value("decisionLinkId").toDouble());
        if (m_decisionsByLink.contains(decisionLinkId)) {
            const LocalDecision old = m_decisionsByLink.take(decisionLinkId);
            for (qint64 linkId : old.corridorLinkIds)
                if (m_decisionOwnerByCorridorLink.value(linkId)
                    == decisionLinkId)
                    m_decisionOwnerByCorridorLink.remove(linkId);
        }
        loadDecision(item);
    }
    return true;
}

bool ProjectModel::loadInitialTraffic(QString* error)
{
    QJsonObject root;
    if (!readJson(QDir(m_root).filePath("generated/initial_traffic_plan.json"), &root, error))
        return false;
    for (const QJsonValue& value : root.value("placements").toArray()) {
        const QJsonObject item = value.toObject();
        InitialPlacement placement;
        placement.placementId = item.value("placementId").toInt(m_placements.size() + 1);
        placement.linkId = static_cast<qint64>(item.value("linkId").toDouble());
        placement.laneNumber = item.value("laneNumber").toInt(0);
        placement.distanceM = item.value("distanceM").toDouble(0.0);
        placement.speedMps = item.value("initialSpeedMps").toDouble(8.0);
        placement.vehicleTypeCode = item.value("vehicleTypeCode").toInt(1);
        placement.vehicleTypeKey = item.value("vehicleTypeKey").toString("PASSENGER_CAV");
        placement.vehicleTypeName = item.value("vehicleTypeName").toString();
        placement.color = item.value("vehicleTypeColor").toString("#3D8BFF");
        placement.originTerminalId = item.value("originTerminalId").toString();
        placement.destinationTerminalId = item.value("destinationTerminalId").toString();
        placement.destinationExitLinkId = static_cast<qint64>(
            item.value("destinationExitLinkId").toDouble());
        if (placement.linkId > 0 && placement.destinationExitLinkId > 0)
            m_placements.push_back(placement);
    }
    return true;
}
