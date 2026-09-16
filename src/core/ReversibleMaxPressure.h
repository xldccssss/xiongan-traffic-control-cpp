#pragma once

#include <QJsonObject>
#include <QString>

enum class PressurePropagationDirection
{
    Forward,
    Reverse,
    Balanced
};

struct ReversiblePressureEvidence
{
    bool enabled = true;
    PressurePropagationDirection direction =
        PressurePropagationDirection::Balanced;
    double forwardPressureVeh = 0.0;
    double reversePressureVeh = 0.0;
    double signedPressureVeh = 0.0;
    double usablePressureVeh = 0.0;
    double queueBackPressureVeh = 0.0;
    double occupancyBackPressureVeh = 0.0;
    double occupancyTrendBackPressureVeh = 0.0;
    double reversePressureShare = 0.0;
    double downstreamOccupancy = 0.0;
    double predictedDownstreamOccupancy = 0.0;
    double downstreamOccupancyTrend = 0.0;
    bool changesVehicleDirection = false;
    bool changesLaneDirection = false;
    bool changesLinkDirection = false;
};

// “可逆”仅表示压力可由拥堵下游向上游传播。该组件不持有 TESSNG
// 对象、不修改车辆/车道方向，也不执行文件、网络或 UI 操作。
class ReversibleMaxPressure
{
public:
    explicit ReversibleMaxPressure(
        const QJsonObject& configuration = QJsonObject());

    void configure(const QJsonObject& configuration);
    ReversiblePressureEvidence evaluate(
        double forwardPressureVeh,
        double downstreamQueueVeh,
        double downstreamQueueWeight,
        double downstreamOccupancy,
        double predictedDownstreamOccupancy,
        double downstreamOccupancyTrend) const;

    bool isEnabled() const { return m_enabled; }
    QString modelId() const { return m_modelId; }
    static QString defaultModelId();
    static QString directionName(PressurePropagationDirection direction);

private:
    bool m_enabled = true;
    QString m_modelId;
    double m_occupancyBackPressureVeh = 6.0;
    double m_occupancyBackPressureExponent = 2.0;
    double m_occupancyTrendBackPressureVeh = 8.0;
    double m_maximumAdditionalBackPressureVeh = 8.0;
    double m_pressureDirectionDeadbandVeh = 0.5;
};
