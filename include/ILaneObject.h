/********************************************************************************
* 车道对象接口，由TESS NG实现，是车道(ILane)和车道连接(ILaneConnector)的基类
* 用户可以借此调用接口方法，获取车道或车道连接的基本属性
*********************************************************************************/

#ifndef __ILaneObject__
#define __ILaneObject__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QList>
#include <QPointF>
#include <QVector3D>
#include <QJsonObject>

class ISection;
class ILane;
class ILaneConnector;

class TESSINTERFACES_EXPORT ILaneObject
{
public:
	virtual ~ILaneObject() = default;

	/*
	获取车道对象类型
	@return 返回GLaneType或GLaneConnectorType枚举值
	*/
	virtual int gtype() = 0;

	/*
	判断是否为车道
	@return 如果是车道返回true，如果是车道连接返回false
	*/
	virtual bool isLane() = 0;

	/*
	获取ID
	如果是Lane，返回Lane的ID；如果是车道连接，返回车道连接ID
	@return 返回车道对象的唯一标识ID
	*/
	virtual long id() = 0;

	/*
	获取长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回车道对象的长度
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取所属Section
	@return 返回所属的ISection接口
	*/
	virtual ISection* section() = 0;

	/*
	根据ID获取上游LaneObject
	如果当前是车道，id为0时返回空指针，否则返回上游指定ID的"车道连接"；
	如果当前是车道连接，id为0时返回上游车道，否则返回空指针
	@param id 车道连接ID，默认为0
	@return 返回上游ILaneObject接口，若无则返回空指针
	*/
	virtual ILaneObject* fromLaneObject(long id = 0) = 0;

	/*
	根据ID获取下游LaneObject
	如果当前是车道，id为0时返回空指针，否则返回下游指定ID的"车道连接"；
	如果当前是车道连接，id为0时返回下游车道，否则返回空指针
	@param id 车道连接ID，默认为0
	@return 返回下游ILaneObject接口，若无则返回空指针
	*/
	virtual ILaneObject* toLaneObject(long id = 0) = 0;

	/*
	获取中心线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> centerBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取左侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> leftBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取右侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> rightBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取中心线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> centerBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取左侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> leftBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取右侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> rightBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取左侧部分断点集（三维坐标）
	@param fromPoint 起点（中心线上的点）
	@param toPoint 终点（中心线上的点）
	@param unit 单位类型，默认为默认单位
	@return 返回指定范围内左侧线断点的三维坐标列表
	*/
	virtual QList<QVector3D> leftBreak3DsPartly(QPointF fromPoint, QPointF toPoint, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取右侧部分断点集（三维坐标）
	@param fromPoint 起点（中心线上的点）
	@param toPoint 终点（中心线上的点）
	@param unit 单位类型，默认为默认单位
	@return 返回指定范围内右侧线断点的三维坐标列表
	*/
	virtual QList<QVector3D> rightBreak3DsPartly(QPointF fromPoint, QPointF toPoint, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	计算中心线上指定点到起点的距离
	@param p 中心线上的点（二维坐标）
	@param unit 单位类型，默认为默认单位(像素)
	@return 返回该点到起点的距离
	*/
	virtual qreal distToStartPoint(const QPointF p, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	计算中心线上指定点到起点的距离（带分段序号）
	附加条件是该点所在车道上的分段序号
	@param p 中心线上的点（二维坐标）
	@param segmIndex 分段序号，默认为0
	@param bOnCentLine 点是否在中心线上，默认为true
	@param unit 单位类型，默认为默认单位
	@return 返回该点到起点的距离
	*/
	virtual qreal distToStartPointWithSegmIndex(const QPointF p, int segmIndex = 0, bool bOnCentLine = true, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	根据距离获取中心线上的点及分段序号
	从中心线起点向前延伸dist距离后，获取所在点坐标及所在分段的序号
	@param dist 距离值
	@param outPoint 输出参数，返回计算得到的点坐标
	@param outIndex 输出参数，返回点所在的分段序号
	@param unit 单位类型，默认为默认单位
	@return 如果目标点在中心线上返回true，否则返回false
	*/
	virtual bool getPointAndIndexByDist(qreal dist, QPointF& outPoint, int& outIndex, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	根据距离获取中心线上的点
	从中心线起点向前延伸dist距离后，获取所在点坐标
	@param dist 距离值
	@param outPoint 输出参数，返回计算得到的点坐标
	@param unit 单位类型，默认为默认单位
	@return 如果目标点在中心线上返回true，否则返回false
	*/
	virtual bool getPointByDist(qreal dist, QPointF& outPoint, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	设置车道或车道连接的其他属性
	@param attr 包含其他属性的JSON对象
	*/
	virtual void setOtherAttr(QJsonObject attr) = 0;

	/*
	将ILaneObject转换为ILane
	如果当前ILaneObject是"车道连接"则返回空指针
	@return 返回ILane接口，如果不是车道则返回空指针
	*/
	virtual ILane* castToLane() = 0;

	/*
	将ILaneObject转换为ILaneConnector
	如果当前ILaneObject是车道则返回空指针
	@return 返回ILaneConnector接口，如果不是车道连接则返回空指针
	*/
	virtual ILaneConnector* castToLaneConnector() = 0;

};

#endif