/********************************************************************************
* 车辆接口，由TESS NG实现，用户可以借此调用接口方法，获取车辆一些基本属性，以及运动过程中的一些动态数据，
* 还可以调用一些接口方法对车辆进行初始化，设置动态信息
*********************************************************************************/

#ifndef IVEHICLE_H
#define IVEHICLE_H

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QObject>
#include <QPicture>
#include <QJsonObject>

class Vehicle;
class IVehicleDriving;
class ILink;
class ILaneObject;
class ILane;
class ILaneConnector;
class IRouting;
class ISection;
class SimulatorBatchCommands;

class TESSINTERFACES_EXPORT IVehicle
{
public:
	// 车辆下一批次控制代理，由所属车辆持有，仅保存车辆 ID。
	class Step
	{
	public:
		TESSINTERFACES_EXPORT void setDesirSpeed(qreal speed);
		TESSINTERFACES_EXPORT void setSpeed(qreal speed);
		TESSINTERFACES_EXPORT void setAcce(qreal acce);
		TESSINTERFACES_EXPORT void setFollowingType(int followingType);
		TESSINTERFACES_EXPORT void setFollowingParam(qreal safeInterval, qreal safeDistance);
		TESSINTERFACES_EXPORT void setFrontVehicle(IVehicle* pVehicleFront);
		TESSINTERFACES_EXPORT void setFrontDistance(qreal frontDistance, qreal stopDistance);
		TESSINTERFACES_EXPORT void setLeftLaneChange(bool enabled);
		TESSINTERFACES_EXPORT void setRightLaneChange(bool enabled);
		TESSINTERFACES_EXPORT void setLeftLaneChangeFreely(bool enabled);
		TESSINTERFACES_EXPORT void setRightLaneChangeFreely(bool enabled);
		TESSINTERFACES_EXPORT void setDismissLaneChange(bool enabled);
		TESSINTERFACES_EXPORT void setIfCalcNextRoad(bool ifCalc);
		TESSINTERFACES_EXPORT void setCandidateLaneConnectors(const QList<ILaneConnector*>& laneConnectors);
		TESSINTERFACES_EXPORT void setNextLaneConnector(ILaneConnector* pLaneConnector);
		TESSINTERFACES_EXPORT void setIfCalcNextPoint(bool ifCalc);
		TESSINTERFACES_EXPORT void setLimitedLanes(const QList<int>& laneNumbers);
		TESSINTERFACES_EXPORT void setMaxLimitedSpeed(qreal speed);
		TESSINTERFACES_EXPORT void setDistanceToEvent(qreal distance);
		TESSINTERFACES_EXPORT void setChangeLaneSafeDistance(qreal distance);
		TESSINTERFACES_EXPORT void setVehicleEuler(const QVector3D& euler);
		TESSINTERFACES_EXPORT void setAngle(qreal angle);

	private:
		Step() = default;

		long mVehicleId = 0;

		friend class IVehicle;
	};

	IVehicle();
	virtual ~IVehicle();

	/*
	获取车辆ID
	@return 返回车辆ID
	*/
	virtual long id();

	/*
	设置车辆ID
	@param id 车辆ID
	*/
	virtual void setId(long id);

	/*
	获取车辆名称
	@return 返回车辆名称
	*/
	virtual QString name();

	/*
	获取车辆长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回车辆长度（像素或米）
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置车辆长度
	@param len 车辆长度
	@param bRestWidth 是否同比例约束宽度，默认为false
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setLength(qreal len, bool bRestWidth = false, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取车辆宽度
	@return 返回车辆宽度（像素或米）
	*/
	virtual qreal width();

	/*
	获取车辆颜色
	@return 返回车辆颜色RGB值，格式如"#EE0000"
	*/
	virtual QString color();

	/*
	设置车辆颜色
	@param color 颜色RGB值，格式如"#EE0000"
	*/
	virtual void setColor(QString color);

	/*
	设置车辆类型
	@param code 车辆类型代码
	*/
	virtual void setVehiType(int code);

	/*
	获取车辆创建时的仿真时间
	@return 返回创建时的仿真时间
	*/
	virtual qreal simuTimeOnCreate();

	/*
	获取车辆停止仿真时的仿真时间
	@return 返回停止时的仿真时间（秒）
	*/
	virtual long simuTimeOnStop();

	/*
	获取基本期望速度
	与IVehicleDriving的desirSpeed()有区别，后者综合考虑了基本期望速度和道路限速
	@return 返回基本期望速度
	*/
	virtual qreal desirSpeed();

	/*
	设置基本期望速度
	@param speed 期望速度值
	*/
	virtual void setDesirSpeed(qreal speed);

	/*
	获取最大速度
	@return 返回最大速度（像素/秒或米/秒）
	*/
	virtual qreal maxSpeed();

	/*
	设置最大速度
	@param maxSpeed 最大速度值
	*/
	virtual void setMaxSpeed(qreal maxSpeed);

	/*
	获取最大加速度
	@return 返回最大加速度
	*/
	virtual qreal maxAcce();

	/*
	设置最大加速度
	@param acce 最大加速度值
	*/
	virtual void setMaxAcce(qreal acce);

	/*
	获取最大减速度
	@return 返回最大减速度
	*/
	virtual qreal maxDece();

	/*
	设置最大减速度
	@param dece 最大减速度值
	*/
	virtual void setMaxDece(qreal dece);

	/*
	获取车辆所在道路最大限速
	与IVehicleDriving的limitMaxSpeed()相同
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回最大限速
	*/
	virtual qreal limitMaxSpeed(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取车辆所在道路最小限速
	与IVehicleDriving的limitMinSpeed()相同
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回最小限速
	*/
	virtual qreal limitMinSpeed(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取期望加速度
	@return 返回期望加速度
	*/
	virtual qreal desirAcce();

	/*
	设置期望加速度
	@param acce 期望加速度值
	*/
	virtual void setDesirAcce(qreal acce);

	/*
	获取期望减速度
	@return 返回期望减速度
	*/
	virtual qreal desireDece();

	/*
	设置期望减速度
	@param dece 期望减速度值
	*/
	virtual void setDesireDece(qreal dece);


	/*
	获取起始路段
	@return 返回起始路段接口指针
	*/
	virtual ILink* startLink();

	/*
	获取起始仿真时间
	@return 返回起始仿真时间
	*/
	virtual long startSimuTime();

	/*
	获取当前道路
	如果在路段上返回ILink指针，如果在连接段上返回IConnector指针
	@return 返回道路接口指针
	*/
	virtual void* road();

	/*
	获取当前道路ID
	@return 返回道路ID
	*/
	virtual long roadId();

	/*
	获取当前路段或连接段接口
	@return 返回ISection接口指针
	*/
	virtual ISection* section();

	/*
	判断当前道路是否为路段
	@return 是路段返回true，否则在连接段上返回false
	*/
	virtual bool roadIsLink();

	/*
	获取道路名称
	@return 返回道路名称
	*/
	virtual QString roadName();

	/*
	获取当前车道或车道连接
	@return 返回ILaneObject接口指针
	*/
	virtual ILaneObject* laneObj();

	/*
	获取当前车道ID
	@return 返回车道ID
	*/
	virtual long laneId();

	/*
	获取连接段的去向路段上车道ID
	@return 返回去向车道ID
	*/
	virtual long toLaneId();

	/*
	获取当前车道
	如果在车道连接上，返回空指针
	@return 返回ILane接口指针
	*/
	virtual ILane* lane();

	/*
	获取当前车道连接的下游车道
	如果当前不在车道连接上，返回空指针
	@return 返回ILane接口指针
	*/
	virtual ILane* toLane();

	/*
	获取当前车道连接
	如果在车道上，返回空指针
	@return 返回ILaneConnector接口指针
	*/
	virtual ILaneConnector* laneConnector();

	/*
	获取当前LaneObject上的分段序号
	@return 返回分段序号
	*/
	virtual int segmIndex();

	/*
	获取当前仿真计算批次
	@return 返回仿真计算批次号
	*/
	virtual long currBatchNumber();

	/*
	获取道路类型
	@return 返回道路类型（路段或连接段）
	*/
	virtual int roadType();

	/*
	获取车道或车道连接中心线内点集
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回点集列表
	*/
	virtual QList<QPointF> lLaneObjectVertex(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取车辆类型代码
	@return 返回车辆类型代码
	*/
	virtual long vehicleTypeCode();

	/*
	获取车辆类型名称
	@return 返回车辆类型名称
	*/
	virtual QString vehicleTypeName();

	/*
	获取车辆驾驶行为接口
	@return 返回IVehicleDriving接口指针
	*/
	virtual IVehicleDriving* vehicleDriving();

	/*
	执行驾驶操作
	*/
	virtual void driving();

	/*
	初始化速度
	@param speed 初始速度，默认-1表示使用默认值
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回初始速度
	*/
	virtual qreal initSpeed(qreal speed = -1, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	在路段上用车道序号、距起点距离、当前速度进行初始化
	@param laneNumber 车道序号
	@param dist 距起点距离，默认-1表示使用默认值
	@param speed 当前速度，默认-1表示使用默认值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void initLane(int laneNumber, qreal dist = -1, qreal speed = -1, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	在连接段上用车道连接起始车道序号和目标车道序号以及距起点距离、当前速度进行初始化
	@param laneNumber 起始车道序号
	@param toLaneNumber 目标车道序号
	@param dist 距起点距离，默认-1表示使用默认值
	@param speed 当前速度，默认-1表示使用默认值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void initLaneConnector(int laneNumber, int toLaneNumber, qreal dist = -1, qreal speed = -1, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置是否使用缓存
	@param bCache 是否使用缓存
	*/
	virtual void useCache(bool bCache);

	/*
	获取当前路径
	@return 返回IRouting接口指针
	*/
	virtual IRouting* routing();

	/*
	获取当前点位
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回当前位置坐标
	*/
	virtual QPointF pos(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前高程
	由当前所在路段或连接段的高程决定
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回当前高程
	*/
	virtual qreal zValue(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前高程
	由当前所在路段或连接段的分段两个端点高程决定
	@return 返回当前高程值
	*/
	virtual qreal v3z();

	/*
	获取当前加速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回当前加速度
	*/
	virtual qreal acce(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回当前速度（像素/秒或米/秒）
	*/
	virtual qreal currSpeed(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前角度
	@return 返回当前角度
	*/
	virtual qreal angle();

	/*
	获取车辆图
	@return 返回车辆图片
	*/
	virtual QPicture picture();

	/*
	判断车辆是否已启动
	@return 已启动返回true，否则返回false
	*/
	virtual bool isStarted();


	/*
	获取前车
	@return 返回前车接口指针
	*/
	virtual IVehicle* vehicleFront();

	/*
	获取后车
	@return 返回后车接口指针
	*/
	virtual IVehicle* vehicleRear();

	/*
	获取左前车
	@return 返回左前车接口指针
	*/
	virtual IVehicle* vehicleLFront();

	/*
	获取左后车
	@return 返回左后车接口指针
	*/
	virtual IVehicle* vehicleLRear();

	/*
	获取右前车
	@return 返回右前车接口指针
	*/
	virtual IVehicle* vehicleRFront();

	/*
	获取右后车
	@return 返回右后车接口指针
	*/
	virtual IVehicle* vehicleRRear();

	/*
	获取前车距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回前车距离（像素或米）
	*/
	virtual qreal vehiDistFront(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取前车速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回前车速度
	*/
	virtual qreal vehiSpeedFront(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取后车距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回后车距离（像素或米）
	*/
	virtual qreal vehiDistRear(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取后车速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回后车速度
	*/
	virtual qreal vehiSpeedRear(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取距前车时距
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距前车时距
	*/
	virtual qreal vehiHeadwayFront(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取距后车时距
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距后车时距
	*/
	virtual qreal vehiHeadwaytoRear(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻左车道前车距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻左车道前车距离（像素或米）
	*/
	virtual qreal vehiDistLLaneFront(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻左车道前车速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻左车道前车速度
	*/
	virtual qreal vehiSpeedLLaneFront(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻左车道后车距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻左车道后车距离
	*/
	virtual qreal vehiDistLLaneRear(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻左车道后车速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻左车道后车速度
	*/
	virtual qreal vehiSpeedLLaneRear(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻右车道前车距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻右车道前车距离（像素或米）
	*/
	virtual qreal vehiDistRLaneFront(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻右车道前车速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻右车道前车速度
	*/
	virtual qreal vehiSpeedRLaneFront(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻右车道后车距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻右车道后车距离
	*/
	virtual qreal vehiDistRLaneRear(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相邻右车道后车速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回相邻右车道后车速度
	*/
	virtual qreal vehiSpeedRLaneRear(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取车辆由方向和长度决定的四个拐角构成的多边形
	@return 返回多边形
	*/
	virtual QPolygonF boundingPolygon();

	/*
	判断是否是营运车辆
	@return 是营运车辆返回true，否则返回false
	*/
	virtual bool isCommercialVehicle();

	/*
	获取核载人数
	@return 返回核载人数
	*/
	virtual int maxPassengerCapacity();

	/*
	获取当前乘客数量（含司机）
	@return 返回当前乘客数量
	*/
	virtual int passengerCount();

	/*
	获取剩余可载乘客数量
	@return 返回剩余可载乘客数量
	*/
	virtual int availableSeatCount();

	/*
	上车n个人
	@param n 要上车的人数
	@return 返回实际上车人数
	*/
	virtual int boardPassengers(int n);

	/*
	下车n个人
	@param n 要下车的人数
	@return 返回实际下车人数
	*/
	virtual int alightPassengers(int n);

	/*
	判断是否已经被预定（营运车辆）
	@return 已被预定返回true，否则返回false
	*/
	virtual bool isHookedUp();

	/*
	设置是否已经被预定
	@param bHookedUp 是否已被预定
	*/
	virtual void setHookedUp(bool bHookedUp);

	/*
	设置标签表示的状态
	@param tag 标签值
	*/
	virtual void setTag(long tag);

	/*
	获取标签表示的状态
	@return 返回标签值
	*/
	virtual long tag();

	/*
	设置文本信息，用于在运行过程保存临时信息，方便开发
	@param text 文本信息
	*/
	virtual void setTextTag(QString text);

	/*
	获取文本信息，运行过程临时保存的信息，方便开发
	@return 返回文本信息
	*/
	virtual QString textTag();

	/*
	设置动态信息
	@param pDynaInfo 动态信息指针
	*/
	virtual void setDynaInfo(void* pDynaInfo);

	/*
	获取动态信息
	@return 返回动态信息指针
	*/
	virtual void* dynaInfo();

	/*
	设置json格式数据
	@param info json格式数据
	*/
	virtual void setJsonInfo(QJsonObject info);

	/*
	获取json格式数据
	@return 返回json格式数据
	*/
	virtual QJsonObject jsonInfo();

	/*
	获取json字段值
	@param propName 字段名称
	@return 返回字段值
	*/
	virtual QVariant jsonProperty(QString propName);

	/*
	设置json数据属性
	@param key 属性键名
	@param value 属性值
	*/
	virtual void setJsonProperty(QString key, QVariant value);

	/*
	获取自定义属性数据
	vehiOtherPropertyType: 二次开发新增类型使用[0-1000], 内部使用VehicleOtherPropertyType中定义类型
	@param vehiOtherPropertyType 属性类型
	@return 返回属性值
	*/
	virtual QVariant otherProperty(int vehiOtherPropertyType);

	/*
	设置自定义属性数据
	vehiOtherPropertyType: 二次开发新增类型使用[0-1000], 内部使用VehicleOtherPropertyType中定义类型
	注意：vehicle不管理传入指针类型对象的生命周期
	@param vehiOtherPropertyType 属性类型
	@param value 属性值
	*/
	virtual void setOtherProperty(int vehiOtherPropertyType, QVariant value);


	//====设置插件方法调用步长，即多少个计算周期调用一次
	//目的在于python环境下设置指定方法调用步长，减少不必要的对python过载方法调用频次，以减少效率的损失

	/*
	设置是否允许客户对于车辆的绘制
	@param bDraw 是否允许绘制
	*/
	virtual void setIsPermitForVehicleDraw(bool bDraw);

	/*
	设置CustomerSimulator::beforeNextPoint方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_beforeNextPoint(int steps);

	/*
	设置CustomerSimulator::nextPoint方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_nextPoint(int steps);

	/*
	设置CustomerSimulator::afterStep方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_afterStep(int steps);

	/*
	设置CustomerSimulator::isStopDriving方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_isStopDriving(int steps);


	/*
	设置CustomerSimulator::judgeIfOnTargetLane方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_judgeIfOnTargetLane(int steps);

	/*
	设置CustomerSimulator::reCalcdesirSpeed方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reCalcdesirSpeed(int steps);

	/*
	设置CustomerSimulator::calcMaxLimitedSpeed方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_calcMaxLimitedSpeed(int steps);

	/*
	设置CustomerSimulator::calcLimitedLaneNumber方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_calcLimitedLaneNumber(int steps);

	/*
	设置CustomerSimulator::calcSpeedLimitByLane方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_calcSpeedLimitByLane(int steps);

	/*
	设置CustomerSimulator::calcDistToEventObj方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_calcDistToEventObj(int steps);

	/*
	设置CustomerSimulator::calcChangeLaneSafeDist方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_calcChangeLaneSafeDist(int steps);

	/*
	设置CustomerSimulator::reCalcToLeftLane方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reCalcToLeftLane(int steps);

	/*
	设置CustomerSimulator::reCalcToRightLane方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reCalcToRightLane(int steps);

	/*
	设置CustomerSimulator::reCalcToLeftFreely方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reCalcToLeftFreely(int steps);

	/*
	设置CustomerSimulator::reCalcToRightFreely方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reCalcToRightFreely(int steps);

	/*
	设置CustomerSimulator::afterCalcTracingType方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_afterCalcTracingType(int steps);

	/*
	设置CustomerSimulator::beforeMergingToLane方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_beforeMergingToLane(int steps);

	/*
	设置CustomerSimulator::reSetFollowingType方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reSetFollowingType(int steps);

	/*
	设置CustomerSimulator::calcAcce方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_calcAcce(int steps);

	/*
	设置CustomerSimulator::reSetAcce方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reSetAcce(int steps);

	/*
	设置CustomerSimulator::reSetSpeed方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reSetSpeed(int steps);

	/*
	设置CustomerSimulator::reCalcAngle方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_reCalcAngle(int steps);

	/*
	设置CustomerSimulator::recentTimeOfSpeedAndPos方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_recentTimeOfSpeedAndPos(int steps);

	/*
	设置CustomerSimulator::travelOnChangingTrace方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_travelOnChangingTrace(int steps);

	/*
	设置CustomerSimulator::leaveOffChangingTrace方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_leaveOffChangingTrace(int steps);

	/*
	设置CustomerSimulator::beforeNextRoad方法的调用步长
	每steps个计算周期调用一次
	@param steps 计算周期数
	*/
	virtual void setSteps_beforeNextRoad(int steps);

	/*
	获取下一批次控制代理。返回指针由当前车辆持有，不能跨越车辆生命周期保留。
	*/
	Step* step();

private:
	static SimulatorBatchCommands* currentStepCommands(const char* methodName);
	Step* mStep = nullptr;

};

#endif // IVEHICLE_H
