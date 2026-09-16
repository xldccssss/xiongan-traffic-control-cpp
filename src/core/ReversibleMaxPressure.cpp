#include "ReversibleMaxPressure.h"

#include <QtGlobal>
#include <cmath>

namespace {
double bounded(double value, double minimum, double maximum)
{
    return qMax(minimum, qMin(maximum, value));
}

double number(const QJsonObject& object, const char* key, double fallback)
{
    return object.value(QLatin1String(key)).toDouble(fallback);
}
}

ReversibleMaxPressure::ReversibleMaxPressure(
    const QJsonObject& configuration)
{
    configure(configuration);
}

QString ReversibleMaxPressure::defaultModelId()
{
    return QStringLiteral("SIGNED_REVERSIBLE_PRESSURE_PROPAGATION_V1");
}

QString ReversibleMaxPressure::directionName(
    PressurePropagationDirection direction)
{
    switch (direction) {
    case PressurePropagationDirection::Forward:
        return QStringLiteral("FORWARD");
    case PressurePropagationDirection::Reverse:
        return QStringLiteral("REVERSE");
    default:
        return QStringLiteral("BALANCED");
    }
}

void ReversibleMaxPressure::configure(const QJsonObject& configuration)
{
    m_enabled = configuration.value(QStringLiteral("enabled")).toBool(true);
    m_modelId = configuration.value(QStringLiteral("modelId"))
        .toString(defaultModelId());
    m_occupancyBackPressureVeh = qMax(
        0.0, number(configuration, "occupancyBackPressureVeh", 6.0));
    m_occupancyBackPressureExponent = qMax(
        1.0, number(configuration, "occupancyBackPressureExponent", 2.0));
    m_occupancyTrendBackPressureVeh = qMax(
        0.0, number(configuration, "occupancyTrendBackPressureVeh", 8.0));
    m_maximumAdditionalBackPressureVeh = qMax(
        0.0, number(configuration, "maximumAdditionalBackPressureVeh", 8.0));
    m_pressureDirectionDeadbandVeh = qMax(
        0.0, number(configuration, "pressureDirectionDeadbandVeh", 0.5));
}

ReversiblePressureEvidence ReversibleMaxPressure::evaluate(
    double forwardPressureVeh,
    double downstreamQueueVeh,
    double downstreamQueueWeight,
    double downstreamOccupancy,
    double predictedDownstreamOccupancy,
    double downstreamOccupancyTrend) const
{
    ReversiblePressureEvidence evidence;
    evidence.enabled = m_enabled;
    evidence.forwardPressureVeh = qMax(0.0, forwardPressureVeh);
    evidence.downstreamOccupancy = bounded(
        downstreamOccupancy, 0.0, 1.0);
    evidence.predictedDownstreamOccupancy = bounded(
        predictedDownstreamOccupancy, 0.0, 1.0);
    evidence.downstreamOccupancyTrend = downstreamOccupancyTrend;
    evidence.queueBackPressureVeh = qMax(0.0, downstreamQueueWeight)
        * qMax(0.0, downstreamQueueVeh);

    if (m_enabled) {
        evidence.occupancyBackPressureVeh = m_occupancyBackPressureVeh
            * std::pow(evidence.predictedDownstreamOccupancy,
                       m_occupancyBackPressureExponent);
        evidence.occupancyTrendBackPressureVeh =
            m_occupancyTrendBackPressureVeh
            * qMax(0.0, downstreamOccupancyTrend);
        const double rawAdditional = evidence.occupancyBackPressureVeh
            + evidence.occupancyTrendBackPressureVeh;
        if (rawAdditional > m_maximumAdditionalBackPressureVeh
            && rawAdditional > 0.0) {
            const double scale = m_maximumAdditionalBackPressureVeh
                / rawAdditional;
            evidence.occupancyBackPressureVeh *= scale;
            evidence.occupancyTrendBackPressureVeh *= scale;
        }
    }

    evidence.reversePressureVeh = evidence.queueBackPressureVeh
        + evidence.occupancyBackPressureVeh
        + evidence.occupancyTrendBackPressureVeh;
    evidence.signedPressureVeh = evidence.forwardPressureVeh
        - evidence.reversePressureVeh;
    evidence.usablePressureVeh = qMax(0.0, evidence.signedPressureVeh);
    if (evidence.signedPressureVeh > m_pressureDirectionDeadbandVeh)
        evidence.direction = PressurePropagationDirection::Forward;
    else if (evidence.signedPressureVeh < -m_pressureDirectionDeadbandVeh)
        evidence.direction = PressurePropagationDirection::Reverse;
    else
        evidence.direction = PressurePropagationDirection::Balanced;

    const double magnitude = evidence.forwardPressureVeh
        + evidence.reversePressureVeh;
    evidence.reversePressureShare = magnitude > 0.0
        ? evidence.reversePressureVeh / magnitude : 0.0;
    return evidence;
}
