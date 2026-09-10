//
// Created by jd on 2023/2/27.
//

#include "tessinterfaces_global.h"

#ifndef TESSNG_ICONJDSWITHTESS_H
#define TESSNG_ICONJDSWITHTESS_H

#include <cstdint>
#include <unordered_map>
#include <memory>

#include <QList>
#include "qobject.h"

#include "ivehicle.h"
#include "Plugin/_datastruct.h"

class ITessClient;
class IDistributedManager;
class IVehicleQueueCounter;
class IVehicleTravelDetector;

class TESSINTERFACES_EXPORT IConjDsWithTess : public QObject {
    Q_OBJECT
public:
    enum State : char {
        NotStarted = 1,
        Running = 2,
        Pause = 4,
        Stop = 8
    };
    virtual ~IConjDsWithTess() = default;
    //virtual bool walk(uint32_t from_step, uint32_t to_step, bool try_lock = false);
    virtual bool walk(uint32_t from_step, uint32_t to_step, bool try_lock = false, const std::function<bool()>& judge_quit_func = []() { return false; });
    virtual bool tessWalkInDistributed(uint32_t from_step, uint32_t to_step);
    virtual bool distributeLinks(QList<long>& lLinkId);
    virtual bool isInnerSec(long linkId);
    virtual bool handleVehiOutSection(IVehicle* pVehi, long outSecId);
    virtual uint handleDrivenInVehi();
    virtual void init(std::weak_ptr<IDistributedManager> pDSMgr, bool needDistributeLinks = true);
    virtual void beforeStart();
    virtual void finalize();
    virtual uint handleTailVehi();
    virtual bool dispatchOnlineMsg(const QString& json_str, QString& type1, QString& type2, QJsonArray& paramArr);
    virtual bool isSelfMaster();
    virtual QSharedPointer<ITessClient> getTraceClient();
    virtual QSharedPointer<ITessClient> getDetClient();

    virtual bool recordSubQueueCalcJob(IVehicleQueueCounter* pQC, ILaneConnector* startLaneConn, long timeId, long simuInterval, double subLength, long subVehiCount);
    virtual void recordLocalQueueCalcRes(IVehicleQueueCounter* pQC, long timeId, long simuInterval, double subLength, long subVehiCount);
    virtual long getFrontSubQueueTimeId(long qcId);
    virtual long getFrontSubQueueVehiCount(long qcId);
    virtual double getFrontSubQueuedLen(long qcId);
    virtual bool isInnerQC(long qcId);

    virtual void addTravelSamples(IVehicleTravelDetector* pTravelDet, long timeId);

    virtual QList<Online::VehiInfoAggregated> getVehiInfoAgg(quint64 batch_num);

    virtual const std::map<quint64, QList<Online::VehiInfoAggregated>>& getVehiInfoAggs() const;

    virtual quint64 getNowBatchNumViaMaster();

    virtual void setRecoverOnSpecifiedBatchCmd(uint64_t specifiedBatch, std::string projId = "", std::string planId = "");

    virtual void stop();

signals:
    void startOnline(quint16 port);
};

#endif //TESSNG_ICONJDSWITHTESS_H
