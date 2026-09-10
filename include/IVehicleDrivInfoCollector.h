/***********************************************************************
 * 车辆驾驶信息采集器接口，TESS NG实现，用户借此接口可以获取和设置采集器的属性
 ***********************************************************************/

#ifndef __IVehicleDrivInfoCollector__
#define __IVehicleDrivInfoCollector__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QObject>
#include <QPolygonF>

class ILink;
class IConnector;
class ILane;
class ILaneConnector;

class TESSINTERFACES_EXPORT IVehicleDrivInfoCollector
{
public:
	virtual ~IVehicleDrivInfoCollector() = default;

	/*
	获取采集器ID
	@return 返回采集器ID
	*/
	virtual long id();

	/*
	获取采集器名称
	@return 返回采集器名称
	*/
	virtual QString collName();

	/*
	判断采集器是否在路段上
	@return 在路段上返回true，否则在连接段上返回false
	*/
	virtual bool onLink();

	/*
	获取采集器所在路段
	@return 返回路段接口指针，如果采集器不在路段上则返回nullptr
	*/
	virtual ILink* link();

	/*
	获取采集器所在连接段
	@return 返回连接段接口指针，如果采集器不在连接段上则返回nullptr
	*/
	virtual IConnector* connector();

	/*
	获取采集器所在车道
	如果采集器在路段上则返回该车道，否则返回nullptr
	@return 返回车道接口指针
	*/
	virtual ILane* lane();

	/*
	获取采集器所在车道连接
	如果采集器在连接段上则返回该车道连接，否则返回nullptr
	@return 返回车道连接接口指针
	*/
	virtual ILaneConnector* laneConnector();

	/*
	获取采集器距离所在车道起点或车道连接起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距离值
	*/
	virtual qreal distToStart(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取采集器所在点位置
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回采集器位置坐标
	*/
	virtual QPointF point(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取采集器工作起始时间
	@return 返回起始时间（秒）
	*/
	virtual long fromTime();

	/*
	获取采集器工作停止时间
	@return 返回停止时间（秒）
	*/
	virtual long toTime();

	/*
	获取集计数据时间间隔
	@return 返回时间间隔（秒）
	*/
	virtual long aggregateInterval();

	/*
	设置采集器名称
	@param name 采集器名称
	*/
	virtual void setName(QString name);

	/*
	设置采集器距车道起点（或车道连接起点）距离
	@param dist 距离值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setDistToStart(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置工作起始时间
	@param time 起始时间（秒）
	*/
	virtual void setFromTime(long time);

	/*
	设置工作结束时间
	@param time 结束时间（秒）
	*/
	virtual void setToTime(long time);

	/*
	设置集计数据时间间隔
	@param interval 时间间隔（秒）
	*/
	virtual void setAggregateInterval(int interval);

	/*
	获取采集器多边形轮廓
	@return 返回多边形轮廓
	*/
	virtual QPolygonF polygon();

	/*
	获取采集器角度
	@return 返回角度值
	*/
	virtual qreal angle();
};


#endif
