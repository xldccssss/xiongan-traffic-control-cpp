#pragma once

#include <QHash>
#include <QJsonObject>
#include <QMutex>
#include <QString>
#include <QVector>

#include "ProjectModel.h"
#include "ReversibleMaxPressure.h"
#include "TrafficAggregator.h"

enum class ControlBranch
{
    FixedTiming,
    ImprovedMaxPressure
};

enum class SignalTransition
{
    Green,
    Yellow,
    AllRed
};

struct IntersectionSignalState
{
    int activeStage = 0;
    int pendingStage = -1;
    SignalTransition transition = SignalTransition::Green;
    double stateStartedSec = 0.0;
    double greenStartedSec = 0.0;
    double lastDecisionSec = -1e9;
    QVector<double> previousQueue;
    QVector<double> previousDownstreamOccupancy;
    QVector<double> previousScore;
    QVector<double> underservedCycles;
    QVector<double> plannedGreenSec;
    QVector<double> lastQueues;
    QVector<double> lastScores;
    QVector<double> lastEffectiveScores;
    QVector<ReversiblePressureEvidence> lastReversiblePressure;
    QVector<double> stageRedStartedSec;
    QVector<double> stageLastGreenStartedSec;
    QVector<double> terminalMultipliers;
    QVector<int> stageSkipCount;
    QVector<int> stageServiceCount;
    QVector<bool> pressureInitialized;
    QVector<double> redAgeSec;
    QVector<bool> spillbackBlocked;
    double plannedCycleSec = 80.0;
    double lastPlanUpdateSec = -1e9;
    double emptyStageStartedSec = -1.0;
    bool edgeIsLeader = false;
    bool authorityInitialized = false;
    qint64 authorityLeaseId = 0;
    double authorityLeaseValidFromSec = 0.0;
    double authorityLeaseUntilSec = 0.0;
    double authorityLastEvaluationSec = -1e9;
    int authorityChallenger = -1; // 0=terminal, 1=edge
    int authorityAdvantageCount = 0;
    qint64 authoritySwitchCount = 0;
};

struct StagePressureEvidence
{
    double queueVeh = 0.0;
    double predictedArrivalsVeh = 0.0;
    double queueTrendVeh = 0.0;
    double predictedDownstreamOccupancy = 0.0;
    double measurementReliability = 0.85;
    double rawWeight = 0.05;
    double weight = 0.05;
    double terminalWeight = 0.05;
    double edgeWeight = 0.05;
    double fusedWeight = 0.05;
    double greenWaveMultiplier = 1.0;
    bool greenWaveArrivalActive = false;
    bool spillbackBlocked = false;
    ReversiblePressureEvidence reversiblePressure;
};

class SignalEngine
{
public:
    explicit SignalEngine(const ProjectModel* model = nullptr);
    void setModel(const ProjectModel* model);
    void reset(double nowSec = 0.0);
    void setBranch(ControlBranch branch);
    ControlBranch branch() const;
    void update(double nowSec, const TrafficAggregator& traffic);

    QString colorForLane(qint64 laneId) const;
    QString colorForStage(int intersectionIndex, int stageIndex) const;
    QString activeStageId(int intersectionIndex) const;
    QString authorityOwner(int intersectionIndex) const;
    double waitForMovement(qint64 fromLinkId, qint64 toLinkId,
                           double nowSec) const;
    int allRedIntersectionCount() const;
    int entryGreenWaveGatewayStageCount() const;
    int entryGreenWaveCorridorStageCount() const;
    int entryGreenWaveArcCount() const;
    const QVector<IntersectionSignalState>& states() const { return m_states; }

private:
    void advanceTransition(int intersectionIndex, double nowSec);
    void updateFixed(int intersectionIndex, double nowSec);
    void updateImproved(int intersectionIndex, double nowSec,
                        const QVector<StageTraffic>& traffic,
                        double networkCongestion,
                        const QVector<double>& intersectionCongestion);
    QVector<StagePressureEvidence> improvedEvidence(
        int intersectionIndex, const QVector<StageTraffic>& traffic,
        double nowSec);
    QVector<double> coordinateAuthority(
        int intersectionIndex, double nowSec,
        const QVector<StageTraffic>& traffic,
        QVector<StagePressureEvidence>* evidence,
        double networkCongestion,
        const QVector<double>& intersectionCongestion);
    QVector<double> effectiveScores(
        int intersectionIndex, double nowSec,
        const QVector<double>& scores,
        const QVector<StageTraffic>& traffic,
        double* softRedSec, double* hardRedSec);
    void updatePlanMemory(int intersectionIndex,
                          const QVector<StageTraffic>& traffic,
                          const QVector<StagePressureEvidence>& evidence,
                          double networkCongestion,
                          double nowSec);
    QVector<double> congestionByIntersection(
        const QVector<QVector<StageTraffic>>& values) const;
    int serviceDebtCandidate(int intersectionIndex,
                             const QVector<int>& candidates,
                             const QVector<double>& effective,
                             const QVector<StageTraffic>& traffic) const;
    void rebuildEntryGreenWaveTopology();
    bool greenWaveArrivalActive(int intersectionIndex, int stageIndex,
                                double nowSec) const;
    bool isEntryGreenWaveGateway(int intersectionIndex, int stageIndex) const;
    void beginSwitch(int intersectionIndex, int nextStage, double nowSec);

    struct GreenWaveArc
    {
        int upstreamIntersection = -1;
        int upstreamStage = -1;
        double travelTimeSec = 0.0;
    };

    const ProjectModel* m_model = nullptr;
    QVector<IntersectionSignalState> m_states;
    ControlBranch m_branch = ControlBranch::ImprovedMaxPressure;
    bool m_permissiveRightTurn = true;
    double m_minGreenSec = 12.0;
    double m_maxGreenSec = 45.0;
    double m_decisionIntervalSec = 5.0;
    double m_switchRatio = 1.15;
    double m_minimumGain = 0.5;
    QJsonObject m_signalConfig;
    QJsonObject m_actuatedConfig;
    QJsonObject m_pressureConfig;
    QJsonObject m_greenWaveConfig;
    QJsonObject m_authorityConfig;
    QJsonObject m_terminalConfig;
    ReversibleMaxPressure m_reversiblePressure;
    QVector<QVector<QVector<GreenWaveArc>>> m_greenWaveSources;
    QVector<QVector<bool>> m_entryGreenWaveGateways;
    QVector<QVector<bool>> m_entryGreenWaveCorridorStages;
    int m_entryGreenWaveGatewayStageCount = 0;
    int m_entryGreenWaveCorridorStageCount = 0;
    int m_entryGreenWaveArcCount = 0;
    qint64 m_authorityLeaseSerial = 0;
    // Lamp-color callbacks may run on TESSNG worker threads while update()
    // advances the control state on the simulation thread.
    mutable QMutex m_mutex{QMutex::Recursive};
};
