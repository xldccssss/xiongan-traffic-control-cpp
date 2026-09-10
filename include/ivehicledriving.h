/********************************************************************************
* 驾驶行为接口，由TESS NG实现，用户可以借此调用接口方法，获取驾驶过程动态数据，
* 还可以调用一些接口方法对改变原有的驾驶行为或参数
*********************************************************************************/

#ifndef IVEHICLEDRIVING_H
#define IVEHICLEDRIVING_H

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QObject>
#include <QGraphicsItem>
#include <QVector3D>

class IVehicle;
class IRouting;
class ISection;
class ILane;
class ILaneConnector;
class ILaneObject;
class ISignalLamp;

class TESSINTERFACES_EXPORT IVehicleDriving
{
public:
	virtual ~IVehicleDriving() = default;

	/*
	获取所属车辆
	@return 返回车辆接口指针
	*/
	virtual IVehicle* vehicle();

	/*
	获取随机数
	@return 返回随机数值
	*/
	virtual long getRandomNumber();

	/*
	计算到下一点
	travelInterval：到下点经历的时间，单位毫秒
	@return 计算成功返回true，否则返回false
	*/
	virtual bool nextPoint();

	/*
	获取当前车速为零的持续时间
	@return 返回持续时间（毫秒）
	*/
	virtual long zeroSpeedInterval();

	/*
	判断当前是否在路径上
	@return 在路径上返回true，否则返回false
	*/
	virtual bool isOnRouting();

	/*
	判断当前是否在路段上且有决策点
	@return 满足条件返回true，否则返回false
	*/
	virtual bool isHavingDeciPointOnLink();

	/*
	停止车辆运行
	*/
	virtual void stopVehicle();

	/*
	获取跟驰类型
	@return 返回跟驰类型
	*/
	virtual int followingType();

	/*
	初始化轨迹
	*/
	virtual void initTrace();

	/*
	设置轨迹
	@param lPoint 轨迹点列表
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setTrace(QList<QPointF>& lPoint, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	计算轨迹长度
	*/
	virtual void calcTraceLength();

	/*
	获取轨迹类型
	@return 返回轨迹类型
	*/
	virtual int tracingType();

	/*
	设置轨迹类型
	@param type 轨迹类型
	*/
	virtual void setTracingType(int type);

	/*
	获取车辆角度
	@return 返回车辆角度
	*/
	virtual qreal angle();

	/*
	设置车辆角度
	@param angle 角度值
	*/
	virtual void setAngle(qreal angle);

	/*
	获取车辆欧拉角
	@param bPositive 是否取正值
	@return 返回欧拉角向量
	*/
	virtual QVector3D euler(bool bPositive = false);

	/*
	获取期望速度
	综合了道路限速，与IVehicle的desirSpeed()有所不同，IVehicle的desirSpeed()是基本期望速度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回期望速度
	*/
	virtual qreal desirSpeed(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取车辆所在道路最大限速
	与IVehicle的limitMaxSpeed()相同
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回最大限速
	*/
	virtual qreal limitMaxSpeed(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取车辆所在道路最小限速
	与IVehicle的limitMinSpeed()相同
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回最小限速
	*/
	virtual qreal limitMinSpeed(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前所在路段或连接段
	@return 返回路段或连接段接口指针
	*/
	virtual ISection* getCurrRoad();

	/*
	获取下一路段或连接段
	@return 返回路段或连接段接口指针
	*/
	virtual ISection* getNextRoad();

	/*
	获取当前受其影响的信号灯
	@return 返回信号灯接口指针
	*/
	virtual ISignalLamp* getCurrSignalLamp();

	/*
	获取与目标车道编号的差值
	@return 返回车道编号差值
	*/
	virtual int differToTargetLaneNumber();

	/*
	执行左变道
	@param bFource 是否强制变道，默认为false
	*/
	virtual void toLeftLane(bool bFource = false);

	/*
	执行右变道
	@param bFource 是否强制变道，默认为false
	*/
	virtual void toRightLane(bool bFource = false);

	/*
	获取当前车道序号
	@return 返回车道序号
	*/
	virtual int laneNumber();

	/*
	设置车道序号
	@param number 车道序号
	*/
	virtual void setLaneNumber(int number);

	/*
	设置当前路段或连接段已行驶的距离
	@param dist 已行驶距离
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setCurrDistanceInRoad(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前路段或连接段已行驶距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回已行驶距离
	*/
	virtual qreal currDistanceInRoad(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置当前分段已行驶的距离
	@param dist 已行驶距离
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setCurrDistanceInSegment(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前分段已行驶距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回已行驶距离
	*/
	virtual qreal currDistanceInSegment(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置分段序号
	@param index 分段序号
	*/
	virtual void setSegmentIndex(int index);

	/*
	设置当前已行驶总里程
	@param dist 总里程
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setVehiDrivDistance(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前已行驶总里程
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回总里程
	*/
	virtual qreal getVehiDrivDistance(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置曲化轨迹上行驶的距离
	@param dist 已行驶距离
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setCurrDistanceInTrace(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置曲化轨迹上的分段序号
	@param index 分段序号
	*/
	virtual void setIndexOfSegmInTrace(int index);

	/*
	设置是否改变轨迹
	@param b 是否改变轨迹
	*/
	virtual void setChangingTracingType(bool b);

	/*
	获取当前时间段移动距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回移动距离
	*/
	virtual qreal currDistance(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置车辆X坐标
	@param posX X坐标值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setX(qreal posX, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置车辆Y坐标
	@param posY Y坐标值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setY(qreal posY, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置车辆Z方向速度分量
	@param v3z Z方向速度值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setV3z(qreal v3z, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置跟驰状态
	@param followingType 跟驰类型
	@param interval 时间间隔，默认为0
	@param pIVehicleNegotiated 协商车辆接口指针，默认为nullptr
	*/
	virtual void setFollowingType(int followingType, int interval = 0, IVehicle* pIVehicleNegotiated = 0);

	/*
	获取协商车辆
	@return 返回协商车辆接口指针
	*/
	virtual IVehicle* getVehicleNegotiated();

	/*
	获取变轨点集
	车辆不在车道中心线或"车道连接"中心线上时的轨迹，如变道过程的轨迹
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回变轨点列表
	*/
	virtual QList<QPointF> changingTrace(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取变轨长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回变轨长度
	*/
	virtual qreal changingTraceLength(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取在车道或车道连接上到起点距离
	@param fromVehiHead 是否从车头计算
	@param bOnCentLine 当前是否在中心线上
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回到起点的距离
	*/
	virtual qreal distToStartPoint(bool fromVehiHead = false, bool bOnCentLine = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取在车道或车道连接上到终点距离
	@param fromVehiHead 是否从车头计算
	@param bOnCentLine 当前是否在中心线上
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回到终点的距离
	*/
	virtual qreal distToEndpoint(bool fromVehiHead = false, bool bOnCentLine = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置路径
	外界设置的路径不一定有决策点，可能是临时创建的
	如果车辆不在此路径上则设置不成功并返回false
	@param pRouting 路径接口指针
	@return 设置成功返回true，否则返回false
	*/
	virtual bool setRouting(IRouting* pRouting);

	/*
	获取当前路径
	@return 返回路径接口指针
	*/
	virtual IRouting* routing();

	/*
	将车辆移到另一条车道上
	@param pLane 目标车道接口指针
	@param dist 到目标车道起点距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 移动成功返回true，否则返回false
	*/
	virtual bool moveToLane(ILane* pLane, qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	将车辆移到另一条车道连接上
	@param pLaneConnector 目标车道连接接口指针
	@param dist 到目标车道连接起点距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 移动成功返回true，否则返回false
	*/
	virtual bool moveToLaneConnector(ILaneConnector* pLaneConnector, qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移动车辆到另一条车道或车道连接
	@param pILaneObject 目标车道或车道连接接口指针
	@param dist 到目标起点距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 移动成功返回true，否则返回false
	*/
	virtual bool move(ILaneObject* pILaneObject, qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取时距
	@return 返回时距值
	*/
	virtual qreal safeTimeInterval();

	/*
	获取停车距离
	@return 返回停车距离
	*/
	virtual qreal stoppingDistance();

	/*
	修改时距
	@param time 时距值
	*/
	virtual void setSafeTimeInterval(qreal time);

	/*
	修改停车距离
	@param dist 停车距离值
	*/
	virtual void setStoppingDistance(qreal dist);

	/*
	判断是否为第一辆车
	@param bCalc 是否实时计算
	@return 是第一辆车返回true，否则返回false
	*/
	virtual bool isFirstVehi(bool bCalc = false);

	/*
	获取车辆距道路起点距离
	@param fromVehiHead 是否从车头计算
	@return 返回距道路起点距离
	*/
	virtual qreal distToRoadStart(bool fromVehiHead = false);

	/*
	获取车辆距道路终点距离
	@param fromVehiHead 是否从车头计算
	@return 返回距道路终点距离
	*/
	virtual qreal distToRoadEnd(bool fromVehiHead = false);

private:
	IVehicle* mpVehicle;
};

#endif // IVEHICLEDRIVING_H