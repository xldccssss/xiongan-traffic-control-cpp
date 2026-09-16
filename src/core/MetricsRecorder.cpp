#include "MetricsRecorder.h"

#include <algorithm>
#include <cmath>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>

#include "ProjectModel.h"
#include "RoutingEngine.h"
#include "SignalEngine.h"
#include "ivehicle.h"

namespace {
double meanOf(const QVector<double>& values)
{ double sum=0.0;for(double value:values)sum+=value;return values.isEmpty()?0.0:sum/values.size(); }
}

MetricsRecorder::MetricsRecorder(const QString& projectRoot,const ProjectModel* model)
    :m_root(projectRoot),m_model(model){}

void MetricsRecorder::start(const QString& algorithm,
                            int vehicleSeed,
                            int destinationSeed,
                            int requestedRound,
                            const QString& comparisonBatchId,
                            int comparisonRoundIndex,
                            int comparisonRoundCount)
{
    m_algorithm=algorithm;m_vehicleSeed=vehicleSeed;m_destinationSeed=destinationSeed;
    m_requestedRound=qMax(0,requestedRound);m_comparisonBatchId=comparisonBatchId;
    m_comparisonRoundIndex=qMax(0,comparisonRoundIndex);
    m_comparisonRoundCount=qMax(0,comparisonRoundCount);
    m_samples.clear();m_vehicleStates.clear();m_completedTrips.clear();
    m_abnormalTrips.clear();m_registeredVehicleIds.clear();m_activeVehiclesAtSnapshot=0;
    m_officialStartSec=m_model?m_model->scenario().value("initialTraffic").toObject()
        .value("preloadWindowSec").toDouble(15.0):15.0;
    m_lastNetworkSec=-1.0;m_observedUntilSec=m_officialStartSec;
    m_previousQueue=m_previousSpillback=m_previousMeanImbalance=0.0;
    m_previousImbalanceExposure=m_previousAllLanesBusy=0.0;
    m_previousGreenStages=m_previousCombinedStages=m_previousFallbackStages=0.0;
    m_previousServedApproaches=0.0;m_queueVehicleSeconds=m_spillbackApproachSeconds=0.0;
    m_laneImbalanceSeconds=m_laneImbalanceExposureSeconds=m_allLanesCongestedSeconds=0.0;
    m_greenStageSeconds=m_combinedStageGreenSeconds=m_protectedFallbackGreenSeconds=0.0;
    m_parallelApproachServiceSeconds=m_concurrentServiceGainSeconds=0.0;
    m_queueSamples.clear();m_spillbackSamples.clear();m_laneImbalanceSamples.clear();m_maxLaneImbalanceSamples.clear();
    m_intersectionDiagnostics.fill(IntersectionMetricDiagnostic(),
        m_model ? m_model->intersections().size() : 0);
    m_eligibleRegistered=m_eligibleAbnormal=m_initialCompleted=m_initialAbnormal=0;
    m_wrongDestinationCount=m_nonTerminalDisappearanceCount=0;
}

void MetricsRecorder::append(const NetworkTelemetry& telemetry)
{
    if(m_samples.isEmpty()||telemetry.simulationTimeSec-m_samples.last().simulationTimeSec>=4.999) {
        NetworkTelemetry sample = telemetry;
        sample.completedVehicles = m_completedTrips.size();
        m_samples.push_back(sample);
    }
}

void MetricsRecorder::finishVehicle(qint64 vehicleId,double now,bool reached,qint64 lastLinkId)
{
    auto it=m_vehicleStates.find(vehicleId);if(it==m_vehicleStates.end())return;
    const VehicleMetricState state=it.value();m_vehicleStates.erase(it);
    if(lastLinkId<=0)lastLinkId=state.lastLinkId;
    reached=reached||(lastLinkId>0&&lastLinkId==state.destinationLinkId);
    if(!state.eligible){if(reached)++m_initialCompleted;else ++m_initialAbnormal;return;}
    if(!reached){
        ++m_eligibleAbnormal;
        QJsonObject row;row["vehicleId"]=static_cast<double>(vehicleId);
        row["originEntryLinkId"]=static_cast<double>(state.originLinkId);
        row["destinationExitLinkId"]=static_cast<double>(state.destinationLinkId);
        row["lastLinkId"]=static_cast<double>(lastLinkId);
        row["departureSimulationSec"]=state.departureSec;
        row["terminationSimulationSec"]=now;
        row["completedNormally"]=false;
        QString reason = QStringLiteral("NON_TERMINAL_DISAPPEARANCE");
        if (lastLinkId == state.originLinkId)
            reason = QStringLiteral("RETURNED_TO_ORIGIN");
        else if (m_model && m_model->forbiddenExitLinkIds().contains(lastLinkId))
            reason = QStringLiteral("EXCLUDED_UNCONFIGURED_EXIT");
        else if (m_model && m_model->terminalIndexByExitLink().contains(lastLinkId))
            reason = QStringLiteral("WRONG_CONFIGURED_TERMINAL");
        if (reason == QStringLiteral("WRONG_CONFIGURED_TERMINAL")
            || reason == QStringLiteral("EXCLUDED_UNCONFIGURED_EXIT")
            || reason == QStringLiteral("RETURNED_TO_ORIGIN"))
            ++m_wrongDestinationCount;
        else if (reason == QStringLiteral("NON_TERMINAL_DISAPPEARANCE"))
            ++m_nonTerminalDisappearanceCount;
        row["terminationReason"] = reason;
        m_abnormalTrips.push_back(row);
        return;
    }
    const double travel=qMax(0.0,now-state.departureSec);
    QJsonObject row;row["vehicleId"]=static_cast<double>(vehicleId);
    row["originEntryLinkId"]=static_cast<double>(state.originLinkId);
    row["destinationExitLinkId"]=static_cast<double>(state.destinationLinkId);
    row["lastLinkId"]=static_cast<double>(lastLinkId);row["departureSimulationSec"]=state.departureSec;
    row["arrivalSimulationSec"]=now;row["travelTimeSec"]=travel;
    if(state.freeFlowSec>=0.0){row["freeFlowTravelTimeSec"]=state.freeFlowSec;row["delaySec"]=qMax(0.0,travel-state.freeFlowSec);}
    row["stopCount"]=state.stopCount;row["queueTimeSec"]=state.queueTimeSec;
    row["sampledDistanceKm"]=state.distanceM/1000.0;row["completedNormally"]=true;
    m_completedTrips.push_back(row);
}

double MetricsRecorder::meanCompletedDelaySec() const
{
    double delaySum = 0.0;
    int delayCount = 0;
    for (const QJsonObject& trip : m_completedTrips) {
        if (!trip.contains("delaySec") || trip.value("delaySec").isNull()) continue;
        delaySum += trip.value("delaySec").toDouble();
        ++delayCount;
    }
    return delayCount > 0 ? delaySum / delayCount : -1.0;
}

int MetricsRecorder::completedDelaySampleCount() const
{
    int delayCount = 0;
    for (const QJsonObject& trip : m_completedTrips)
        if (trip.contains("delaySec") && !trip.value("delaySec").isNull()) ++delayCount;
    return delayCount;
}

void MetricsRecorder::observe(const QList<IVehicle*>& vehicles,const TrafficAggregator& traffic,
    RoutingEngine& routing,const SignalEngine& signalEngine,double now)
{
    if(!m_model)return;
    const QJsonObject metricConfig=m_model->scenario()
        .value("experimentEvaluation").toObject();
    const double queueSpeed=metricConfig.value("queueSpeedThresholdMps").toDouble(2.0);
    const double stopSpeed=metricConfig.value("stopSpeedThresholdMps").toDouble(0.5);
    const double resumeSpeed=qMax(stopSpeed,metricConfig.value("resumeSpeedThresholdMps").toDouble(1.5));
    const double maximumGap=qMax(1.0,metricConfig.value("maximumSampleGapSec").toDouble(5.0));
    const double disappearanceGraceSec=qMax(0.0,
        metricConfig.value("vehicleDisappearanceGraceSec").toDouble(2.5));
    const QHash<qint64, VehicleOdState> odSnapshot = routing.vehicleOdSnapshot();
    for(auto it=odSnapshot.cbegin();it!=odSnapshot.cend();++it){
        if(m_vehicleStates.contains(it.key()))continue;const VehicleOdState&od=it.value();VehicleMetricState state;
        state.vehicleId=it.key();state.originLinkId=od.originEntryLinkId;state.destinationLinkId=od.destinationExitLinkId;
        state.departureSec=od.departureSec;state.freeFlowSec=routing.freeFlowTravelTimeSec(state.originLinkId,state.destinationLinkId);
        state.eligible=!od.isInitialVehicle;
        if(state.eligible&&!m_registeredVehicleIds.contains(it.key())){
            m_registeredVehicleIds.insert(it.key());++m_eligibleRegistered;}
        m_vehicleStates.insert(it.key(),state);
    }
    QSet<qint64> live;
    for(IVehicle* vehicle:vehicles){if(!vehicle)continue;const qint64 id=vehicle->id();live.insert(id);
        auto it=m_vehicleStates.find(id);if(it==m_vehicleStates.end())continue;VehicleMetricState&state=it.value();
        const double speed=qMax(0.0,static_cast<double>(vehicle->currSpeed(Metric)));
        state.missingSinceSec=-1.0;
        if(vehicle->roadIsLink()){
            const qint64 currentLinkId=vehicle->roadId();
            if(state.lastLinkId>0&&state.lastLinkId!=currentLinkId){
                QSet<int> counted;
                const QVector<StageRef> refs=m_model->movementStagesByLink()
                    .value(state.lastLinkId).value(currentLinkId);
                for(const StageRef&ref:refs){
                    if(ref.intersection<0||ref.intersection>=m_intersectionDiagnostics.size()
                        ||counted.contains(ref.intersection))continue;
                    counted.insert(ref.intersection);
                    IntersectionMetricDiagnostic&diagnostic=
                        m_intersectionDiagnostics[ref.intersection];
                    ++diagnostic.sampledMovementPassages;
                    const IntersectionSignalState signal=signalEngine.states()
                        .value(ref.intersection);
                    if(signal.transition==SignalTransition::Green
                        &&signal.activeStage==ref.stage)
                        ++diagnostic.sampledGreenStagePassages;
                }
            }
            state.lastLinkId=currentLinkId;
        }
        if(state.lastSampleSec>=0.0){const double dt=now-state.lastSampleSec;if(dt>0.0&&dt<=maximumGap){
            state.distanceM+=0.5*(state.lastSpeedMps+speed)*dt;if(speed<=queueSpeed)state.queueTimeSec+=dt;
            if(state.hasMoved&&!state.wasStopped&&speed<=stopSpeed){++state.stopCount;state.wasStopped=true;}}}
        if(speed>=resumeSpeed){state.hasMoved=true;state.wasStopped=false;}state.lastSampleSec=now;state.lastSpeedMps=speed;
    }
    QVector<qint64> ended;
    for(auto it=m_vehicleStates.begin();it!=m_vehicleStates.end();++it){
        if(it.value().lastSampleSec<0.0||live.contains(it.key()))continue;
        if(it.value().missingSinceSec<0.0)it.value().missingSinceSec=now;
        if(now-it.value().missingSinceSec+1e-9>=disappearanceGraceSec)
            ended.push_back(it.key());
    }
    for(qint64 id:ended){const VehicleMetricState state=m_vehicleStates.value(id);
        finishVehicle(id,now,state.lastLinkId==state.destinationLinkId,state.lastLinkId);
        // A TESSNG allVehiStarted snapshot can temporarily omit a vehicle
        // while it is transferred through a connector.  Keep its fixed OD
        // state so a later reappearance can never resume as an unowned random
        // route.  Confirmed exits are removed by leaveForNextLaneObj.
    }
    m_activeVehiclesAtSnapshot=live.size();

    if(now+1e-9<m_officialStartSec)return;
    double spillback=0.0;const double spillThreshold=m_model->scenario().value("signals").toObject()
        .value("spillbackThreshold").toDouble(0.82);
    for(const QVector<StageTraffic>& stages:traffic.stages())for(const StageTraffic&stage:stages)
        if(stage.downstreamOccupancy>=spillThreshold)spillback+=1.0;
    const double diagnosticDt=m_lastNetworkSec>=0.0
        ?now-m_lastNetworkSec:0.0;
    if(diagnosticDt>0.0&&diagnosticDt<=maximumGap){
        const QVector<QVector<StageTraffic>>&allStageTraffic=traffic.stages();
        const QVector<IntersectionSignalState>&signalStates=signalEngine.states();
        for(int i=0;i<m_model->intersections().size()
            &&i<m_intersectionDiagnostics.size();++i){
            IntersectionMetricDiagnostic&diagnostic=m_intersectionDiagnostics[i];
            const IntersectionDefinition&intersection=m_model->intersections()[i];
            QSet<qint64> incomingLanes;
            bool intersectionSpillback=false;
            for(int s=0;s<intersection.stages.size();++s){
                for(qint64 laneId:intersection.stages[s].incomingLaneIds)
                    incomingLanes.insert(laneId);
                if(allStageTraffic.value(i).value(s).downstreamOccupancy
                    >=spillThreshold)intersectionSpillback=true;
            }
            double queue=0.0;
            for(qint64 laneId:incomingLanes)
                queue+=traffic.lanes().value(laneId).queueCount;
            diagnostic.maximumQueueVeh=qMax(diagnostic.maximumQueueVeh,queue);
            if(intersectionSpillback)diagnostic.spillbackSec+=diagnosticDt;
            const IntersectionSignalState state=signalStates.value(i);
            qint64 serviceCount=0;
            for(int count:state.stageServiceCount)serviceCount+=qMax(0,count);
            diagnostic.lastStageServiceCount=serviceCount;
            if(state.transition!=SignalTransition::Green)continue;
            diagnostic.greenObservedSec+=diagnosticDt;
            const StageTraffic active=allStageTraffic.value(i)
                .value(state.activeStage);
            if(active.queueVeh>=0.5)diagnostic.greenQueuedSec+=diagnosticDt;
            if(active.queueVeh>=1.0&&active.meanSpeedMps<=stopSpeed
                &&now-state.greenStartedSec>=2.0)
                diagnostic.greenLowSpeedQueueSec+=diagnosticDt;
        }
    }
    QVector<double> imbalances;double exposure=0.0,allBusy=0.0;
    const double imbalanceThreshold=metricConfig.value("laneImbalanceThresholdVeh").toDouble(2.0);
    const double allBusyThreshold=metricConfig.value("allLanesQueueThresholdVeh").toDouble(2.0);
    for(auto fromIt=m_model->movementLanesByLink().cbegin();fromIt!=m_model->movementLanesByLink().cend();++fromIt)
        for(auto toIt=fromIt.value().cbegin();toIt!=fromIt.value().cend();++toIt){const QSet<qint64> laneIds=toIt.value();if(laneIds.size()<=1)continue;
            double minimum=1e30,maximum=0.0,sum=0.0;for(qint64 laneId:laneIds){const double q=traffic.lanes().value(laneId).queueCount;minimum=qMin(minimum,q);maximum=qMax(maximum,q);sum+=q;}
            Q_UNUSED(sum);const double imbalance=maximum-minimum;imbalances.push_back(imbalance);
            if(imbalance>=imbalanceThreshold)exposure+=1.0;if(minimum>=allBusyThreshold)allBusy+=1.0;}
    const double meanImbalance=meanOf(imbalances);
    double maxImbalance=0.0;for(double value:imbalances)maxImbalance=qMax(maxImbalance,value);
    double green=0.0,combined=0.0,fallback=0.0,served=0.0;
    for(int i=0;i<signalEngine.states().size();++i){const IntersectionSignalState&state=signalEngine.states()[i];
        if(state.transition!=SignalTransition::Green)continue;const StageDefinition stage=m_model->intersections()[i].stages.value(state.activeStage);
        green+=1.0;served+=qMax(1,stage.concurrentApproaches);if(stage.concurrentApproaches>1)combined+=1.0;if(stage.sharedLaneSafetyFallback)fallback+=1.0;}
    if(m_lastNetworkSec>=0.0){const double dt=now-m_lastNetworkSec;if(dt>0.0&&dt<=maximumGap){
        m_queueVehicleSeconds+=m_previousQueue*dt;m_spillbackApproachSeconds+=m_previousSpillback*dt;
        m_laneImbalanceSeconds+=m_previousMeanImbalance*dt;m_laneImbalanceExposureSeconds+=m_previousImbalanceExposure*dt;
        m_allLanesCongestedSeconds+=m_previousAllLanesBusy*dt;m_greenStageSeconds+=m_previousGreenStages*dt;
        m_combinedStageGreenSeconds+=m_previousCombinedStages*dt;m_protectedFallbackGreenSeconds+=m_previousFallbackStages*dt;
        m_parallelApproachServiceSeconds+=m_previousServedApproaches*dt;
        m_concurrentServiceGainSeconds+=qMax(0.0,m_previousServedApproaches-m_previousGreenStages)*dt;}}
    m_lastNetworkSec=now;m_observedUntilSec=qMax(m_observedUntilSec,now);
    m_previousQueue=traffic.telemetry().totalQueueVeh;m_previousSpillback=spillback;
    m_previousMeanImbalance=meanImbalance;m_previousImbalanceExposure=exposure;m_previousAllLanesBusy=allBusy;
    m_previousGreenStages=green;m_previousCombinedStages=combined;m_previousFallbackStages=fallback;m_previousServedApproaches=served;
    m_queueSamples.push_back(m_previousQueue);m_spillbackSamples.push_back(spillback);
    m_laneImbalanceSamples.push_back(meanImbalance);m_maxLaneImbalanceSamples.push_back(maxImbalance);
}

double MetricsRecorder::percentile(QVector<double> values,double percent)
{
    if(values.isEmpty())return -1.0;std::sort(values.begin(),values.end());if(values.size()==1)return values.first();
    const double position=qBound(0.0,percent,100.0)/100.0*(values.size()-1);const int lo=static_cast<int>(std::floor(position)),hi=static_cast<int>(std::ceil(position));
    return values[lo]*(1.0-(position-lo))+values[hi]*(position-lo);
}

QJsonObject MetricsRecorder::performance()const
{
    QVector<double> travel,delay,stops,queueTime;for(const QJsonObject&row:m_completedTrips){travel.push_back(row.value("travelTimeSec").toDouble());
        if(row.contains("delaySec"))delay.push_back(row.value("delaySec").toDouble());stops.push_back(row.value("stopCount").toDouble());queueTime.push_back(row.value("queueTimeSec").toDouble());}
    const double duration=qMax(0.0,m_observedUntilSec-m_officialStartSec);QJsonObject out;
    auto optional=[&](const char*key,double value){if(value>=0.0)out[key]=value;else out[key]=QJsonValue();};
    out["schemaVersion"]="experiment-kpi-v1";out["controlBranch"]=m_algorithm;out["officialStartSec"]=m_officialStartSec;out["observedDurationSec"]=duration;
    out["eligibleRegisteredVehicles"]=m_eligibleRegistered;out["eligibleCompletedTrips"]=m_completedTrips.size();out["eligibleAbnormalTrips"]=m_eligibleAbnormal;
    out["activeVehiclesAtSnapshot"]=m_activeVehiclesAtSnapshot;out["completionRate"]=m_completedTrips.size()/double(qMax(1,m_eligibleRegistered));
    out["abnormalTerminationRate"]=m_eligibleAbnormal/double(qMax(1,m_eligibleAbnormal+m_completedTrips.size()));
    out["abnormalShareOfRegistered"]=m_eligibleAbnormal/double(qMax(1,m_eligibleRegistered));
    QJsonObject abnormalReasons;
    for(const QJsonObject&trip:m_abnormalTrips){
        const QString reason=trip.value("terminationReason")
            .toString(QStringLiteral("UNCLASSIFIED"));
        abnormalReasons[reason]=abnormalReasons.value(reason).toInt()+1;
    }
    out["abnormalTerminationReasons"]=abnormalReasons;
    optional("completedTripsPerHour",duration>0?m_completedTrips.size()*3600.0/duration:-1.0);
    optional("meanTravelTimeSec",travel.isEmpty()?-1.0:meanOf(travel));optional("p95TravelTimeSec",percentile(travel,95));
    optional("meanDelaySec",delay.isEmpty()?-1.0:meanOf(delay));optional("p95DelaySec",percentile(delay,95));
    optional("meanStopsPerTrip",stops.isEmpty()?-1.0:meanOf(stops));optional("p95StopsPerTrip",percentile(stops,95));
    optional("meanQueueTimePerTripSec",queueTime.isEmpty()?-1.0:meanOf(queueTime));
    optional("timeMeanTotalQueueVeh",duration>0?m_queueVehicleSeconds/duration:-1.0);optional("p95TotalQueueVeh",percentile(m_queueSamples,95));
    optional("maximumTotalQueueVeh",m_queueSamples.isEmpty()?-1.0:*std::max_element(m_queueSamples.cbegin(),m_queueSamples.cend()));
    out["spillbackApproachSeconds"]=m_spillbackApproachSeconds;optional("p95SpillbackApproaches",percentile(m_spillbackSamples,95));
    optional("timeMeanLaneQueueImbalanceVeh",duration>0?m_laneImbalanceSeconds/duration:-1.0);optional("p95LaneQueueImbalanceVeh",percentile(m_laneImbalanceSamples,95));
    optional("maximumLaneQueueImbalanceVeh",m_maxLaneImbalanceSamples.isEmpty()?-1.0:*std::max_element(m_maxLaneImbalanceSamples.cbegin(),m_maxLaneImbalanceSamples.cend()));
    out["laneImbalanceExposureSeconds"]=m_laneImbalanceExposureSeconds;out["allLanesCongestedMovementSeconds"]=m_allLanesCongestedSeconds;
    out["greenStageSeconds"]=m_greenStageSeconds;out["combinedStageGreenSeconds"]=m_combinedStageGreenSeconds;out["protectedFallbackGreenSeconds"]=m_protectedFallbackGreenSeconds;
    out["parallelApproachServiceSeconds"]=m_parallelApproachServiceSeconds;out["concurrentServiceGainApproachSeconds"]=m_concurrentServiceGainSeconds;
    optional("combinedStageGreenShare",m_greenStageSeconds>0?m_combinedStageGreenSeconds/m_greenStageSeconds:-1.0);
    optional("protectedFallbackGreenShare",m_greenStageSeconds>0?m_protectedFallbackGreenSeconds/m_greenStageSeconds:-1.0);
    out["uncertifiedConflictStageSeconds"]=0.0;out["initialVehiclesCompleted"]=m_initialCompleted;out["initialVehiclesAbnormal"]=m_initialAbnormal;
    if(!m_samples.isEmpty()){const NetworkTelemetry&last=m_samples.last();optional("energyPer100KmKwh",last.distanceKm>0?last.energyKWh*100.0/last.distanceKm:-1.0);}
    return out;
}

int MetricsRecorder::chooseRound(const QString& algorithm)const
{const QDir resultDir(QDir(m_root).filePath("result"));for(int round=1;round<10000;++round){const QString name=QString("round_%1").arg(round,3,10,QChar('0'));if(!QDir(resultDir.filePath(name+"/"+algorithm)).exists())return round;}return 10000;}

int MetricsRecorder::nextComparisonRound()const
{
    const QDir resultDir(QDir(m_root).filePath("result"));
    for(int round=1;round<10000;++round){
        const QString name=QString("round_%1").arg(round,3,10,QChar('0'));
        if(!QDir(resultDir.filePath(name+"/IMPROVED_MAX_PRESSURE")).exists()
            &&!QDir(resultDir.filePath(name+"/FIXED_TIMING")).exists())return round;
    }
    return 10000;
}

QString MetricsRecorder::finish(const RoutingEngine* routing,QString* error)
{
    if(m_samples.isEmpty()&&m_completedTrips.isEmpty())return QString();
    const int round=m_requestedRound>0?m_requestedRound:chooseRound(m_algorithm);
    const QString relative=QString("result/round_%1/%2").arg(round,3,10,QChar('0')).arg(m_algorithm);
    const QString directory=QDir(m_root).filePath(relative);if(!QDir().mkpath(directory)){if(error)*error=directory;return QString();}
    QJsonArray series;for(const NetworkTelemetry&sample:m_samples){QJsonObject row;row["simulationTimeSec"]=sample.simulationTimeSec;row["runningVehicles"]=sample.runningVehicles;
        row["generatedVehicles"]=static_cast<double>(sample.generatedVehicles);row["completedVehicles"]=static_cast<double>(sample.completedVehicles);row["removedVehicles"]=static_cast<double>(sample.removedVehicles);row["meanSpeedKph"]=sample.meanSpeedKph;
        row["queuedVehicles"]=sample.queuedVehicles;row["energyKWh"]=sample.energyKWh;row["distanceKm"]=sample.distanceKm;row["actualMultiple"]=sample.actualMultiple;
        row["wrongExitBlockedVehicles"]=sample.wrongExitBlockedVehicles;row["pendingGreenReleaseVehicles"]=sample.pendingGreenReleaseVehicles;
        row["connectorStallVehicles"]=sample.connectorStallVehicles;row["routeMaintenanceFailureCount"]=sample.routeMaintenanceFailureCount;
        row["stopLineLaneChangeDismissVehicles"]=static_cast<double>(sample.stopLineLaneChangeDismissVehicles);
        row["greenReleaseImpulseCount"]=static_cast<double>(sample.greenReleaseImpulseCount);
        row["connectorDeadlockReleaseImpulseCount"]=static_cast<double>(sample.connectorDeadlockReleaseImpulseCount);series.append(row);}
    QJsonArray trips;for(const QJsonObject&row:m_completedTrips)trips.append(row);
    QJsonArray abnormalTrips;for(const QJsonObject&row:m_abnormalTrips)abnormalTrips.append(row);QJsonObject document;
    const QJsonObject performanceResult=performance();
    document["schemaVersion"]="experiment-result-v2";document["controlBranch"]=m_algorithm;document["performance"]=performanceResult;document["series"]=series;document["completedTrips"]=trips;document["abnormalTrips"]=abnormalTrips;
    QJsonObject controlMethod;
    if (m_algorithm == QStringLiteral("IMPROVED_MAX_PRESSURE")) {
        const QJsonObject pressure = m_model
            ? m_model->scenario().value(QStringLiteral("runtimeControl"))
                  .toObject().value(QStringLiteral("improvedMaxPressure"))
                  .toObject()
            : QJsonObject();
        const QJsonObject reversible = pressure
            .value(QStringLiteral("reversibleMaxPressure")).toObject();
        controlMethod["modelId"] = pressure.value(QStringLiteral("modelId"))
            .toString(QStringLiteral(
                "REVERSIBLE_PRESSURE_CONTINUITY_STORAGE_CONSTRAINED_"
                "CONFLICT_GRAPH_RSU_IMPROVED_MAX_PRESSURE_V5"));
        controlMethod["reversiblePressureModelId"] = reversible
            .value(QStringLiteral("modelId"))
            .toString(QStringLiteral(
                "SIGNED_REVERSIBLE_PRESSURE_PROPAGATION_V1"));
        controlMethod["reversiblePressureEnabled"] = reversible
            .value(QStringLiteral("enabled")).toBool(true);
        controlMethod["changesVehicleDirection"] = false;
        controlMethod["changesLaneDirection"] = false;
        controlMethod["changesLinkDirection"] = false;
    } else {
        controlMethod["modelId"] = QStringLiteral("FIXED_TIMING_BASELINE");
        controlMethod["reversiblePressureEnabled"] = false;
    }
    document["controlMethod"] = controlMethod;
    QJsonArray intersectionDiagnostics;
    qint64 totalSampledPassages=0,totalSampledGreenPassages=0;
    double totalGreenLowSpeedQueueSec=0.0;
    for(int i=0;i<m_intersectionDiagnostics.size();++i){
        const IntersectionMetricDiagnostic&diagnostic=m_intersectionDiagnostics[i];
        QJsonObject row;
        row["intersectionId"]=m_model&&i<m_model->intersections().size()
            ?m_model->intersections()[i].id:QString::number(i);
        row["maximumQueueVeh"]=diagnostic.maximumQueueVeh;
        row["greenObservedSec"]=diagnostic.greenObservedSec;
        row["greenQueuedSec"]=diagnostic.greenQueuedSec;
        row["greenLowSpeedQueueSec"]=diagnostic.greenLowSpeedQueueSec;
        row["greenLowSpeedQueueShare"]=diagnostic.greenQueuedSec>0.0
            ?QJsonValue(diagnostic.greenLowSpeedQueueSec/diagnostic.greenQueuedSec)
            :QJsonValue();
        row["spillbackSec"]=diagnostic.spillbackSec;
        row["sampledMovementPassages"]=static_cast<double>(diagnostic.sampledMovementPassages);
        row["sampledGreenStagePassages"]=static_cast<double>(diagnostic.sampledGreenStagePassages);
        row["lastStageServiceCount"]=static_cast<double>(diagnostic.lastStageServiceCount);
        intersectionDiagnostics.append(row);
        totalSampledPassages+=diagnostic.sampledMovementPassages;
        totalSampledGreenPassages+=diagnostic.sampledGreenStagePassages;
        totalGreenLowSpeedQueueSec+=diagnostic.greenLowSpeedQueueSec;
    }
    document["intersectionSignalDiagnostics"]=intersectionDiagnostics;
    QJsonObject experiment;experiment["roundId"]=QString("round_%1").arg(round,3,10,QChar('0'));
    experiment["vehicleGenerationRandomSeed"]=m_vehicleSeed>=0?QJsonValue(m_vehicleSeed):(m_model?m_model->scenario().value("randomSeeds").toObject().value("vehicleGeneration"):QJsonValue());
    experiment["destinationChoiceRandomSeed"]=m_destinationSeed>=0?QJsonValue(m_destinationSeed):(m_model?m_model->scenario().value("randomSeeds").toObject().value("destinationChoice"):QJsonValue());
    experiment["executionMode"]=m_comparisonBatchId.isEmpty()?QStringLiteral("MANUAL_SINGLE_RUN"):QStringLiteral("AUTOMATIC_PAIRED_COMPARISON");
    if(!m_comparisonBatchId.isEmpty()){
        experiment["comparisonBatchId"]=m_comparisonBatchId;
        experiment["comparisonRoundIndex"]=m_comparisonRoundIndex;
        experiment["comparisonRoundCount"]=m_comparisonRoundCount;
    }
    document["experiment"]=experiment;
    if(!m_samples.isEmpty()){
        const NetworkTelemetry&last=m_samples.last();
        QJsonObject session;session["distanceKm"]=last.distanceKm;session["tractionEnergyKwh"]=last.tractionEnergyKWh;
        session["auxiliaryEnergyKwh"]=last.auxiliaryEnergyKWh;session["regeneratedEnergyKwh"]=last.regeneratedEnergyKWh;
        session["netEnergyKwh"]=last.energyKWh;session["energyPer100KmKwh"]=last.distanceKm>1e-9?QJsonValue(last.energyKWh*100.0/last.distanceKm):QJsonValue();
        session["estimatedCarbonKg"]=last.estimatedCarbonKg;
        QJsonObject instant;instant["tractionPowerKw"]=last.instantTractionPowerKw;instant["auxiliaryPowerKw"]=last.instantAuxiliaryPowerKw;
        instant["regenerationPowerKw"]=last.instantRegenerationPowerKw;instant["netBatteryPowerKw"]=last.instantNetPowerKw;
        QJsonObject energy;energy["modelId"]="EV_LONGITUDINAL_POWER_V1";energy["resultType"]="MODEL_ESTIMATE_NOT_METER_READING";
        energy["activeVehicles"]=last.runningVehicles;energy["meanSpeedKph"]=last.meanSpeedKph;energy["instant"]=instant;energy["session"]=session;document["energy"]=energy;
        QJsonObject routingResult;routingResult["routeDecisionCount"]=routing?static_cast<double>(routing->routeDecisionCount()):0.0;
        routingResult["routeChangeCount"]=routing?static_cast<double>(routing->routeChangeCount()):0.0;routingResult["nativeRouteSyncCount"]=routing?static_cast<double>(routing->nativeRouteSyncCount()):0.0;routingResult["fallbackCount"]=routing?static_cast<double>(routing->routeFallbackCount()):0.0;document["routing"]=routingResult;
        QJsonObject summary;summary["configuredIntersections"]=m_model?m_model->intersections().size():0;summary["reportingIntersections"]=m_model?m_model->intersections().size():0;
        summary["activeVehicles"]=last.runningVehicles;summary["connectedVehicles"]=last.runningVehicles;summary["totalQueueVeh"]=last.totalQueueVeh;
        summary["meanSpeedKph"]=last.meanSpeedKph;summary["instantNetPowerKw"]=last.instantNetPowerKw;summary["netEnergyKwh"]=last.energyKWh;
        summary["regeneratedEnergyKwh"]=last.regeneratedEnergyKWh;summary["energyPer100KmKwh"]=session.value("energyPer100KmKwh");summary["estimatedCarbonKg"]=last.estimatedCarbonKg;
        summary["dynamicRouteDecisions"]=routingResult.value("routeDecisionCount");summary["dynamicRouteChanges"]=routingResult.value("routeChangeCount");summary["routingFallbacks"]=routingResult.value("fallbackCount");
        summary["actualCompletedVehicles"]=m_completedTrips.size();summary["abnormalTerminatedVehicles"]=m_eligibleAbnormal;
        summary["removedVehicles"]=static_cast<double>(last.removedVehicles);
        summary["wrongExitBlockedVehicles"]=last.wrongExitBlockedVehicles;summary["pendingGreenReleaseVehicles"]=last.pendingGreenReleaseVehicles;
        summary["connectorStallVehicles"]=last.connectorStallVehicles;summary["routeMaintenanceFailureCount"]=last.routeMaintenanceFailureCount;
        summary["stopLineLaneChangeDismissVehicles"]=static_cast<double>(last.stopLineLaneChangeDismissVehicles);
        summary["greenReleaseImpulseCount"]=static_cast<double>(last.greenReleaseImpulseCount);
        summary["connectorDeadlockReleaseImpulseCount"]=static_cast<double>(last.connectorDeadlockReleaseImpulseCount);
        summary["sampledIntersectionMovementPassages"]=static_cast<double>(totalSampledPassages);
        summary["sampledGreenStagePassages"]=static_cast<double>(totalSampledGreenPassages);
        summary["greenLowSpeedQueueSeconds"]=totalGreenLowSpeedQueueSec;
        for(const QString&key:{QString("eligibleCompletedTrips"),QString("completionRate"),QString("completedTripsPerHour"),QString("meanTravelTimeSec"),QString("p95TravelTimeSec"),QString("meanDelaySec"),QString("p95DelaySec"),QString("meanStopsPerTrip"),QString("timeMeanTotalQueueVeh"),QString("p95TotalQueueVeh"),QString("spillbackApproachSeconds"),QString("timeMeanLaneQueueImbalanceVeh"),QString("p95LaneQueueImbalanceVeh"),QString("laneImbalanceExposureSeconds"),QString("allLanesCongestedMovementSeconds")})summary[key]=performanceResult.value(key);
        document["summary"]=summary;
    }
    for(const QString&name:{QStringLiteral("experiment_result.json"),QStringLiteral("metrics.json")}){QFile file(QDir(directory).filePath(name));if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate)){if(error)*error=file.errorString();return QString();}file.write(QJsonDocument(document).toJson(QJsonDocument::Indented));}
    QFile csv(QDir(directory).filePath("timeseries.csv"));if(csv.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text)){QTextStream stream(&csv);stream.setCodec("UTF-8");stream<<"simulation_time_sec,running_vehicles,generated_vehicles,completed_vehicles,removed_vehicles,mean_speed_kph,queued_vehicles,energy_kwh,distance_km,actual_multiple,wrong_exit_blocked_vehicles,pending_green_release_vehicles,connector_stall_vehicles,route_maintenance_failure_count,stop_line_lane_change_dismiss_vehicles,green_release_impulse_count,connector_deadlock_release_impulse_count\n";for(const NetworkTelemetry&s:m_samples)stream<<s.simulationTimeSec<<','<<s.runningVehicles<<','<<s.generatedVehicles<<','<<s.completedVehicles<<','<<s.removedVehicles<<','<<s.meanSpeedKph<<','<<s.queuedVehicles<<','<<s.energyKWh<<','<<s.distanceKm<<','<<s.actualMultiple<<','<<s.wrongExitBlockedVehicles<<','<<s.pendingGreenReleaseVehicles<<','<<s.connectorStallVehicles<<','<<s.routeMaintenanceFailureCount<<','<<s.stopLineLaneChangeDismissVehicles<<','<<s.greenReleaseImpulseCount<<','<<s.connectorDeadlockReleaseImpulseCount<<'\n';}
    return QDir::toNativeSeparators(directory);
}
