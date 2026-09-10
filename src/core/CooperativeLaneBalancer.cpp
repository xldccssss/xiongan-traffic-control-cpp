#include "CooperativeLaneBalancer.h"

#include <algorithm>
#include <cmath>

#include <IConnector.h>
#include <ILane.h>
#include <ILaneConnector.h>
#include <ILink.h>
#include <netinterface.h>

namespace {
double value(const QJsonObject& object, const char* key, double fallback)
{ return object.value(QLatin1String(key)).toDouble(fallback); }
}

CooperativeLaneBalancer::CooperativeLaneBalancer(const ProjectModel* model)
{ setModel(model); }

void CooperativeLaneBalancer::setModel(const ProjectModel* model)
{
    m_model = model;
    m_config = model ? model->scenario().value("cav").toObject()
        .value("laneBalancing").toObject() : QJsonObject();
    reset();
}

void CooperativeLaneBalancer::reset()
{
    m_vehicleStates.clear();
    m_requestCount = 0;
    m_completedCount = 0;
}

void CooperativeLaneBalancer::cancelVehicleCommand(qint64 vehicleId)
{
    auto it = m_vehicleStates.find(vehicleId);
    if (it == m_vehicleStates.end()) return;
    it->hasCommand = false;
}

bool CooperativeLaneBalancer::configure(NetInterface* network)
{
    m_lanesByMovement = m_model ? m_model->movementLanesByLink()
                               : QHash<qint64, QHash<qint64, QSet<qint64>>>();
    m_lockedLinkIds.clear();
    m_laneOrderByLink.clear();
    m_lanePosition.clear();
    m_leftNeighbor.clear();
    m_rightNeighbor.clear();
    m_wideningCorridorCount = 0;
    m_wideningUnresolvedCount = 0;
    if (!m_model || !network || !m_config.value("enabled").toBool(true))
        return false;
    buildWideningPolicy(network);
    buildLaneOrders(network);
    return !m_laneOrderByLink.isEmpty();
}

void CooperativeLaneBalancer::buildWideningPolicy(NetInterface* network)
{
    if (!m_config.value("upstreamWideningPreparationEnabled").toBool(true))
        return;
    const bool lockAfter = m_config.value("lockLaneChangesAfterWidening").toBool(true);
    for (const LocalDecision& decision : m_model->decisionsByLink()) {
        if (!decision.relocatedBeforeLaneWidening || decision.corridorLinkIds.size() < 2)
            continue;
        ++m_wideningCorridorCount;
        if (lockAfter)
            for (int i = 1; i < decision.corridorLinkIds.size(); ++i)
                m_lockedLinkIds.insert(decision.corridorLinkIds[i]);
        const qint64 finalLink = decision.corridorLinkIds.last();
        for (const LocalRouteOption& route : decision.routes) {
            const qint64 target = route.routingChoiceLinkId > 0
                ? route.routingChoiceLinkId : route.nextLinkId;
            QSet<qint64> allowed = m_lanesByMovement.value(finalLink).value(target);
            if (allowed.isEmpty()) continue;
            m_lanesByMovement[finalLink][target].unite(allowed);
            bool complete = true;
            for (int i = decision.corridorLinkIds.size() - 2; i >= 0; --i) {
                const qint64 fromLinkId = decision.corridorLinkIds[i];
                const qint64 toLinkId = decision.corridorLinkIds[i + 1];
                ILink* fromLink = network->findLink(fromLinkId);
                QSet<qint64> upstream;
                if (fromLink) {
                    for (ILane* lane : fromLink->lanes()) {
                        if (!lane) continue;
                        for (ILaneConnector* connector : lane->toLaneConnectors()) {
                            if (!connector || !connector->connector()
                                || !connector->connector()->toLink()
                                || connector->connector()->toLink()->id() != toLinkId
                                || !connector->toLane()) continue;
                            if (allowed.contains(connector->toLane()->id()))
                                upstream.insert(lane->id());
                        }
                    }
                }
                if (upstream.isEmpty()) { complete = false; break; }
                allowed = upstream;
                m_lanesByMovement[fromLinkId][target].unite(allowed);
            }
            if (!complete) {
                m_lanesByMovement[decision.decisionLinkId].remove(target);
                ++m_wideningUnresolvedCount;
            }
        }
    }
}

void CooperativeLaneBalancer::buildLaneOrders(NetInterface* network)
{
    QSet<qint64> requiredLinks = m_lockedLinkIds;
    for (auto from = m_lanesByMovement.cbegin(); from != m_lanesByMovement.cend(); ++from)
        requiredLinks.insert(from.key());
    for (qint64 linkId : requiredLinks) {
        ILink* link = network->findLink(linkId);
        if (!link) continue;
        struct Row { qint64 id; QPointF end; QPointF direction; };
        QVector<Row> rows;
        QPointF meanDirection;
        for (ILane* lane : link->lanes()) {
            if (!lane) continue;
            const QList<QPointF> points = lane->centerBreakPoints(Metric);
            if (points.size() < 2) continue;
            QPointF direction = points.last() - points[points.size() - 2];
            const double norm = std::hypot(direction.x(), direction.y());
            if (norm <= 1e-8) continue;
            direction /= norm;
            rows.push_back({lane->id(), points.last(), direction});
            meanDirection += direction;
        }
        const double norm = std::hypot(meanDirection.x(), meanDirection.y());
        if (rows.isEmpty() || norm <= 1e-8) continue;
        const QPointF left(-meanDirection.y() / norm, meanDirection.x() / norm);
        std::stable_sort(rows.begin(), rows.end(), [left](const Row& a, const Row& b) {
            const double pa = QPointF::dotProduct(a.end, left);
            const double pb = QPointF::dotProduct(b.end, left);
            if (std::abs(pa - pb) > 1e-9) return pa > pb;
            return a.id > b.id;
        });
        QVector<qint64> order;
        for (int i = 0; i < rows.size(); ++i) {
            order.push_back(rows[i].id);
            m_lanePosition.insert(rows[i].id, {linkId, i});
            if (i > 0) m_leftNeighbor.insert(rows[i].id, rows[i - 1].id);
            if (i + 1 < rows.size()) m_rightNeighbor.insert(rows[i].id, rows[i + 1].id);
        }
        m_laneOrderByLink.insert(linkId, order);
    }
}

CooperativeLaneBalancer::LaneMetric CooperativeLaneBalancer::metric(
    qint64 laneId, const TrafficAggregator& traffic) const
{
    const LaneTraffic raw = traffic.lanes().value(laneId);
    LaneMetric result;
    result.vehicleCount = qMax(0, raw.vehicleCount);
    result.queue = qMax(0, raw.queueCount);
    result.occupancy = qBound(0.0, raw.occupancy, 1.0);
    const double freeFlow = value(m_config, "freeFlowSpeedMps", 13.9);
    result.speed = raw.vehicleCount > 0 ? qMax(0.0, raw.meanSpeedMps) : freeFlow;
    result.score = value(m_config, "queueWeight", 3.0) * result.queue
        + value(m_config, "vehicleCountWeight", 0.35) * result.vehicleCount
        + value(m_config, "occupancyWeight", 2.0) * result.occupancy
        + value(m_config, "speedDeficitWeight", 0.15)
            * qMax(0.0, freeFlow - result.speed);
    return result;
}

LaneBalanceAction CooperativeLaneBalancer::directionTowardAllowed(
    qint64 currentLaneId, const QSet<qint64>& allowed) const
{
    LaneBalanceAction result;
    if (!m_lanePosition.contains(currentLaneId)) return result;
    const LanePosition current = m_lanePosition.value(currentLaneId);
    int bestDistance = 1000000, bestIndex = current.index;
    qint64 bestLane = 0;
    for (qint64 laneId : allowed) {
        const LanePosition candidate = m_lanePosition.value(laneId, {0, -1});
        if (candidate.linkId != current.linkId || candidate.index < 0) continue;
        const int distance = qAbs(candidate.index - current.index);
        if (distance < bestDistance || (distance == bestDistance && candidate.index < bestIndex)) {
            bestDistance = distance; bestIndex = candidate.index; bestLane = laneId;
        }
    }
    Q_UNUSED(bestLane);
    if (bestIndex < current.index) {
        result.direction = LaneChangeDirection::Left;
        result.targetLaneId = m_leftNeighbor.value(currentLaneId);
    } else if (bestIndex > current.index) {
        result.direction = LaneChangeDirection::Right;
        result.targetLaneId = m_rightNeighbor.value(currentLaneId);
    }
    return result;
}

LaneBalanceAction CooperativeLaneBalancer::queueBalance(
    qint64 currentLinkId, qint64 currentLaneId, const QSet<qint64>& allowed,
    const TrafficAggregator& traffic) const
{
    LaneBalanceAction result;
    const LaneMetric current = metric(currentLaneId, traffic);
    // Optional queue balancing must never start inside a nearly stationary,
    // densely occupied lane. A half-completed change would consume two lanes
    // and reduce capacity more than the balancing can recover.
    if (current.speed < value(m_config, "minimumOptionalLaneChangeSpeedMps", 3.0)
        || current.occupancy > value(
            m_config, "maximumOptionalLaneChangeOccupancy", 0.65))
        return result;
    struct Candidate { LaneMetric metric; qint64 laneId; LaneChangeDirection direction; };
    QVector<Candidate> candidates;
    const qint64 left = m_leftNeighbor.value(currentLaneId);
    const qint64 right = m_rightNeighbor.value(currentLaneId);
    if (left > 0 && allowed.contains(left)) candidates.push_back({metric(left, traffic), left, LaneChangeDirection::Left});
    if (right > 0 && allowed.contains(right)) candidates.push_back({metric(right, traffic), right, LaneChangeDirection::Right});
    if (candidates.isEmpty()) return result;
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (std::abs(a.metric.score - b.metric.score) > 1e-9) return a.metric.score < b.metric.score;
        return a.laneId < b.laneId;
    });
    const Candidate best = candidates.first();
    const double queueGain = current.queue - best.metric.queue;
    const double countGain = current.vehicleCount - best.metric.vehicleCount;
    const double scoreGain = current.score - best.metric.score;
    const double ratio = qBound(0.50, value(m_config, "queueSwitchRatio", 0.75), 0.95);
    const bool ratioOk = current.queue <= 0.0 || best.metric.queue <= current.queue * ratio;
    const bool material = queueGain >= value(m_config, "minimumQueueGainVeh", 2.0)
        || countGain >= value(m_config, "minimumVehicleCountGainVeh", 3.0);
    if (material && ratioOk && scoreGain >= value(m_config, "minimumScoreGain", 2.0)) {
        result.direction = best.direction;
        result.targetLaneId = best.laneId;
        result.currentLinkId = currentLinkId;
    }
    return result;
}

LaneBalanceAction CooperativeLaneBalancer::choose(
    qint64 vehicleId, qint64 currentLinkId, qint64 currentLaneId,
    qint64 targetLinkId, double distanceToEndM, double nowSec,
    const TrafficAggregator& traffic)
{
    LaneBalanceAction none;
    if (!m_config.value("enabled").toBool(true) || targetLinkId <= 0)
        return none;
    VehicleState& state = m_vehicleStates[vehicleId];
    if (state.currentLinkId != currentLinkId) {
        state = VehicleState();
        state.currentLinkId = currentLinkId;
    }
    if (m_lockedLinkIds.contains(currentLinkId)) {
        state.hasCommand = false;
        return none;
    }
    const QSet<qint64> allowed = m_lanesByMovement.value(currentLinkId).value(targetLinkId);
    const double freezeDistance = value(
        m_config, "freezeDistanceBeforeStopLineM", 60.0);
    // A command issued immediately outside the freeze boundary can still be
    // active after the vehicle enters the stop-line protection area and then
    // physically occupy two lanes. Reserve enough distance to finish it.
    const double initiationCutoff = qMax(
        freezeDistance,
        value(m_config, "laneChangeInitiationCutoffBeforeStopLineM",
              freezeDistance + 30.0));
    if (allowed.isEmpty()
        || distanceToEndM > value(m_config, "controlRadiusBeforeStopLineM", 320.0)
        || distanceToEndM < initiationCutoff)
        return none;
    if (state.hasCommand) {
        if (currentLaneId != state.command.fromLaneId) {
            ++m_completedCount;
            state.hasCommand = false;
            state.cooldownUntilSec = nowSec + value(m_config, "postChangeCooldownSec", 5.0);
        } else if (nowSec <= state.command.holdUntilSec) {
            return state.command;
        } else {
            state.hasCommand = false;
            state.cooldownUntilSec = qMax(state.cooldownUntilSec,
                nowSec + value(m_config, "retryCooldownSec", 2.0));
        }
    }
    if (nowSec < state.cooldownUntilSec) return none;
    const bool mandatory = !allowed.contains(currentLaneId);
    // A value of zero deliberately disables route-neutral queue balancing.
    // This is useful on saturated approaches where even a beneficial-looking
    // optional manoeuvre can stop halfway and physically consume two lanes.
    // Mandatory OD alignment remains available, but is issued at most once by
    // the default configuration and only in the upstream preparation zone.
    const int maximumAttempts = qMax(mandatory ? 1 : 0,
        static_cast<int>(value(
            m_config, mandatory ? "maximumRouteAlignmentAttemptsPerLink"
                                : "maximumQueueBalanceAttemptsPerLink",
            mandatory ? 1.0 : 0.0)));
    if (maximumAttempts <= 0) return none;
    if (state.attemptsOnCurrentLink >= maximumAttempts) return none;
    LaneBalanceAction result = mandatory
        ? directionTowardAllowed(currentLaneId, allowed)
        : queueBalance(currentLinkId, currentLaneId, allowed, traffic);
    if (!result.valid()) return none;
    result.mandatory = mandatory;
    result.reason = mandatory ? "ROUTE_ALIGNMENT" : "QUEUE_BALANCE";
    result.fromLaneId = currentLaneId;
    result.currentLinkId = currentLinkId;
    result.movementTargetLinkId = targetLinkId;
    result.issuedAtSec = nowSec;
    result.holdUntilSec = nowSec + value(m_config, "commandHoldSec", 1.5);
    state.command = result;
    state.hasCommand = true;
    ++state.attemptsOnCurrentLink;
    ++m_requestCount;
    return result;
}
