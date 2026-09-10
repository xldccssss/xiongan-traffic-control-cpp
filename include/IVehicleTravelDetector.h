/***********************************************************************
 * 行程时间检测器接口，TESS NG实现，用户借此接口可以获取和设置行程时间检测器的属性
 ***********************************************************************/

#ifndef __IVehicleTravelDetector__
#define __IVehicleTravelDetector__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QObject>
#include <QPolygonF>

class ILink;
class ILaneConnector;

class TESSINTERFACES_EXPORT IVehicleTravelDetector
{
public:
	virtual ~IVehicleTravelDetector() = default;

	/*
	获取采集器ID
	@return 返回采集器ID
	*/
	virtual long id();

	/*
	获取采集器名称
	@return 返回采集器名称
	*/
	virtual QString detectorName();

	/*
	判断是否为检测器起始点
	@return 是起始点返回true，否则返回false
	*/
	virtual bool isStartDetector();

	/*
	判断检测器起点是否在路段上
	@return 在路段上返回true，否则在连接段上返回false
	*/
	virtual bool isOnLink_startDetector();

	/*
	判断检测器终点是否在路段上
	@return 在路段上返回true，否则在连接段上返回false
	*/
	virtual bool isOnLink_endDetector();

	/*
	获取检测器起点所在路段
	如果检测器起点在路段上则返回该路段，否则返回nullptr
	@return 返回路段接口指针
	*/
	virtual ILink* link_startDetector();

	/*
	获取检测器起点所在车道连接
	如果检测器起点在连接段上则返回该车道连接，否则返回nullptr
	@return 返回车道连接接口指针
	*/
	virtual ILaneConnector* laneConnector_startDetector();

	/*
	获取检测器终点所在路段
	如果检测器终点在路段上则返回该路段，否则返回nullptr
	@return 返回路段接口指针
	*/
	virtual ILink* link_endDetector();

	/*
	获取检测器终点所在车道连接
	如果检测器终点在连接段上则返回该车道连接，否则返回nullptr
	@return 返回车道连接接口指针
	*/
	virtual ILaneConnector* laneConnector_endDetector();

	/*
	获取检测器起点距离所在车道起点或车道连接起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距离值
	*/
	virtual qreal distance_startDetector(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取检测器终点距离所在车道起点或车道连接起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距离值
	*/
	virtual qreal distance_endDetector(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取检测器起点位置
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回起点坐标
	*/
	virtual QPointF point_startDetector(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取检测器终点位置
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回终点坐标
	*/
	virtual QPointF point_endDetector(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取检测器工作起始时间
	@return 返回起始时间（秒）
	*/
	virtual long fromTime();

	/*
	获取检测器工作停止时间
	@return 返回停止时间（秒）
	*/
	virtual long toTime();

	/*
	获取集计数据时间间隔
	@return 返回时间间隔（秒）
	*/
	virtual long aggregateInterval();

	/*
	设置检测器名称
	@param name 检测器名称
	*/
	virtual void setName(QString name);

	/*
	设置检测器起点距车道起点（或车道连接起点）距离
	@param dist 距离值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setDistance_startDetector(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置检测器终点距车道起点（或车道连接起点）距离
	@param dist 距离值
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setDistance_endDetector(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

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
	获取行程时间检测器起始点多边形轮廓
	@return 返回多边形轮廓
	*/
	virtual QPolygonF polygon_startDetector();

	/*
	获取行程时间检测器终止点多边形轮廓
	@return 返回多边形轮廓
	*/
	virtual QPolygonF polygon_endDetector();

	/*
	获取检测器角度
	@return 返回角度值
	*/
	virtual qreal angle();
};

#endif