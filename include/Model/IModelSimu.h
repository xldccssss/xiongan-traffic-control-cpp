/******************************************************************************
* 模型仿真接口，由TESS NG实现，用于管理仿真过程中的车辆行为和事件处理
*******************************************************************************/

#ifndef __IModelSimu__
#define __IModelSimu__

#include "tessinterfaces_global.h"

#include <QObject>

class IVehicle;
class ILaneConnector;
class IBusStationLine;
class ILaneObject;
class IDecisionPoint;
class IRouting;

class TESSINTERFACES_EXPORT IModelSimu
{
public:
    /*
    构造函数
    */
    IModelSimu() {}

    /*
    析构函数
    */
    virtual ~IModelSimu() {}

    /*
    仿真前的准备
    @param keepOn 是否继续仿真，如果设置为false则停止仿真
    */
    virtual void beforeStart(bool& keepOn);

    /*
    仿真结束后的处理
    */
    virtual void afterStop();

    /*
    一个批次计算后的处理
    */
    virtual void afterOneStep();

    /*
    单个车辆计算后的处理
    @param pIVehicle 车辆接口指针
    */
    virtual void afterStep(IVehicle* pIVehicle);

    /*
    计算加速度
    @param pIVehicle 车辆接口指针
    @param acce 输出加速度值
    @return 计算成功返回true，否则返回false
    */
    virtual bool calcAcce(IVehicle* pIVehicle, qreal& acce);

    /*
    重新设置加速度
    @param pIVehicle 车辆接口指针
    @param inOutAcce 输入输出加速度值
    @return 设置成功返回true，否则返回false
    */
    virtual bool reSetAcce(IVehicle* pIVehicle, qreal& inOutAcce);

    /*
    重新设置速度
    @param pIVehicle 车辆接口指针
    @param inOutSpeed 输入输出速度值
    @return 设置成功返回true，否则返回false
    */
    virtual bool reSetSpeed(IVehicle* pIVehicle, qreal& inOutSpeed);

    /*
    初始化车辆
    @param pIVehicle 车辆接口指针
    */
    virtual void initVehicle(IVehicle* pIVehicle);

    /*
    到达下一个路径点前的处理
    @param pIVehicle 车辆接口指针
    @param keepOn 是否继续，如果设置为false则停止
    */
    virtual void beforeNextPoint(IVehicle* pIVehicle, bool& keepOn);

    /*
    计算车辆后续"车道连接"
    此时车辆正跨出当前路段，驶到pPreLaneConnector，可以通过此方法改变车辆后续方向。
    如果返回的"车道连接"不在原有路径上，车辆在下一个路段尽头会消失。
    为了解决这个问题可以在返回新的"车道连接"前重新设置路径（经过返回的"车道连接"），或将路径设为空。
    @param pIVehicle 车辆接口指针
    @param pPreLaneConnector 前一个车道连接接口指针
    @return 返回候选车道连接接口指针
    */
    virtual ILaneConnector* candidateLaneConnector(IVehicle* pIVehicle, ILaneConnector* pPreLaneConnector);

    /*
    自由左变道前处理
    如果bKeepOn被赋值为false，TESSNG不再计算是否自由左变道
    @param pIVehicle 车辆接口指针
    @param bKeepOn 是否继续变道判断
    */
    virtual void beforeToLeftFreely(IVehicle* pIVehicle, bool& bKeepOn);

    /*
    自由右变道前处理
    如果bKeepOn被赋值为false，TESSNG不再计算是否自由右变道
    @param pIVehicle 车辆接口指针
    @param bKeepOn 是否继续变道判断
    */
    virtual void beforeToRightFreely(IVehicle* pIVehicle, bool& bKeepOn);

    /*
    计算限制车道序号
    如管制、危险等，最右侧编号为0
    @param pIVehicle 车辆接口指针
    @return 返回限制车道序号列表
    */
    virtual QList<int> calcLimitedLaneNumber(IVehicle* pIVehicle);

    /*
    计算安全变道距离
    @param pIVehicle 车辆接口指针
    @param dist 输出安全变道距离
    @return 计算成功返回true，否则返回false
    */
    virtual bool calcChangeLaneSafeDist(IVehicle* pIVehicle, qreal& dist);

    /*
    仿真结束后插件写文件方法
    @param dataDir 数据文件夹路径
    */
    virtual void writeSimuResult(QString dataDir);

    /*
    离开轨迹
    @param pIVehicle 车辆接口指针
    @param differ 差异值
    @param s 输出距离值
    @return 处理成功返回true，否则返回false
    */
    virtual bool leaveOffChangingTrace(IVehicle* pIVehicle, qreal differ, qreal& s) ;

    /*
    公交到站
    @param pIVehicle 公交车接口指针
    @param pStationLine 站点线路接口指针
    @param alightingCount 下客人数
    @param alightingTime 下客所需总时间
    @param boardingCount 上客人数
    @param boardingTime 上客所需总时间
    */
    virtual void busArriving(IVehicle* pIVehicle, IBusStationLine* pStationLine, int alightingCount, int alightingTime, int boardingCount, int boardingTime);

    /*
    公交离站
    @param pIVehicle 公交车接口指针
    @param pStationLine 站点线路接口指针
    */
    virtual void busOnLeaving(IVehicle* pIVehicle, IBusStationLine* pStationLine);

    /*
    重新计算角度
    @param pIVehicle 车辆接口指针
    @param outAngle 输出角度值
    @return 计算成功返回true，否则返回false
    */
    virtual bool reCalcAngle(IVehicle* pIVehicle, qreal& outAngle);

    /*
    判断是否应该变道接客
    针对营运车辆
    @param pIVehicle 车辆接口指针
    @param laneNumber 输出应该变道的目标车道序号
    @return 应该变道返回true，否则返回false
    */
    virtual bool shouldHookUpPedestrian(IVehicle* pIVehicle, long& laneNumber);

    /*
    离开当前ILaneObject去另一ILaneObject时调用
    如果pAnotherLaneObj为空，表明车辆正驶离路网
    @param pIVehicle 车辆接口指针
    @param pCurrLaneObj 当前车道对象接口指针
    @param pNextLaneObj 下一个车道对象接口指针
    */
    virtual void leaveForNextLaneObj(IVehicle* pIVehicle, ILaneObject* pCurrLaneObj, ILaneObject* pNextLaneObj);

    /*
    离开当前道路前的处理
    @param pIVehicle 车辆接口指针
    @param keepOn 是否继续，如果设置为false则停止
    @return 处理成功返回true，否则返回false
    */
    virtual bool beforeLeaveCurrRoad(IVehicle* pIVehicle, bool& keepOn);

    /*
    分配决策路径后的处理
    @param pIVehicle 车辆接口指针
    @param pIDeciPoint 决策点接口指针
    @param pIRouting 路径接口指针
    */
    virtual void afterDistriRouting(IVehicle* pIVehicle, IDecisionPoint* pIDeciPoint, IRouting* pIRouting);

    /*
    与工作线程同步
    不安全，但效率高
    */
    virtual void duringOneStep();
};

#endif