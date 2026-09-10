/********************************************************************************
* 车道连接接口，由TESS NG实现，用户可以借此调用接口方法，获取车道连接一些基本属性
*********************************************************************************/

#ifndef __ILaneConnector__
#define __ILaneConnector__

#include "tessinterfaces_global.h"
#include "ILaneObject.h"
#include "UnitChange.h"

#include <QObject>
#include <QJsonObject>

class ILane;
class IConnector;

class TESSINTERFACES_EXPORT ILaneConnector : public ILaneObject
{
public:
	virtual ~ILaneConnector() = default;
	//============继承自 ILaneObject 的方法============
	/*
	获取车道连接类型（GLaneConnectorType）
	@return 返回车道连接类型枚举值
	*/
	virtual int gtype() = 0;
	/*
	获取车道连接 ID
	@return 返回车道连接的唯一标识 ID
	*/
	virtual long id() = 0;
	/*
	获取所属 Section
	@return 返回车道连接所属的 ISection 接口
	*/
	virtual ISection* section() = 0;
	/*
	获取车道连接长度
	@param unit 单位类型，默认为默认单位
	@return 返回车道连接的长度
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	根据 ID 获取上游 LaneObject
	id 为 0 时返回上游车道，否则返回空指针
	@param id 车道 ID，默认为 0
	@return 返回上游 ILaneObject 接口，若无则返回空指针
	*/
	virtual ILaneObject* fromLaneObject(long id = 0) = 0;
	/*
	根据 ID 获取下游 LaneObject
	id 为 0 时返回下游车道，否则返回空指针
	@param id 车道 ID，默认为 0
	@return 返回下游 ILaneObject 接口，若无则返回空指针
	*/
	virtual ILaneObject* toLaneObject(long id = 0) = 0;
	/*
	获取车道连接中心线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> centerBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道连接左侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> leftBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道连接右侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> rightBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道连接中心线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> centerBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道连接左侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> leftBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道连接右侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> rightBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道连接左侧部分断点集（三维坐标）
	@param fromPoint 起点（中心线上的点）
	@param toPoint 终点（中心线上的点）
	@param unit 单位类型，默认为默认单位
	@return 返回指定范围内左侧线断点的三维坐标列表
	*/
	virtual QList<QVector3D> leftBreak3DsPartly(QPointF fromPoint, QPointF toPoint, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道连接右侧部分断点集（三维坐标）
	@param fromPoint 起点（中心线上的点）
	@param toPoint 终点（中心线上的点）
	@param unit 单位类型，默认为默认单位
	@return 返回指定范围内右侧线断点的三维坐标列表
	*/
	virtual QList<QVector3D> rightBreak3DsPartly(QPointF fromPoint, QPointF toPoint, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	计算中心线上指定点到起点的距离
	@param p 中心线上的点（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回该点到车道连接起点的距离
	*/
	virtual qreal distToStartPoint(const QPointF p, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	计算中心线上指定点到起点的距离（带分段序号）
	附加条件是该点所在车道上的分段序号
	@param p 中心线上的点（二维坐标）
	@param segmIndex 分段序号，默认为 0
	@param bOnCentLine 点是否在中心线上，默认为 true
	@param unit 单位类型，默认为默认单位
	@return 返回该点到车道连接起点的距离
	*/
	virtual qreal distToStartPointWithSegmIndex(const QPointF p, int segmIndex = 0, bool bOnCentLine = true, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	根据距离获取中心线上的点及分段序号
	从中心线起点向前延伸 dist 距离后，获取所在点坐标及所在分段的序号
	@param dist 距离值
	@param outPoint 输出参数，返回计算得到的点坐标
	@param outIndex 输出参数，返回点所在的分段序号
	@param unit 单位类型，默认为默认单位
	@return 如果目标点在中心线上返回 true，否则返回 false
	*/
	virtual bool getPointAndIndexByDist(qreal dist, QPointF& outPoint, int& outIndex, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	根据距离获取中心线上的点
	从中心线起点向前延伸 dist 距离后，获取所在点坐标
	@param dist 距离值
	@param outPoint 输出参数，返回计算得到的点坐标
	@param unit 单位类型，默认为默认单位
	@return 如果目标点在中心线上返回 true，否则返回 false
	*/
	virtual bool getPointByDist(qreal dist, QPointF& outPoint, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	设置车道连接的其他属性
	@param attr 包含车道连接其他属性的 JSON 对象
	*/
	virtual void setOtherAttr(QJsonObject attr) = 0;


	//============车道连接特有方法============
	/*
	获取所在连接段
	@return 返回车道连接所属的 IConnector 接口
	*/
	virtual IConnector* connector() = 0;
	/*
	获取上游车道
	@return 返回上游的 ILane 接口
	*/
	virtual ILane* fromLane() = 0;
	/*
	获取下游车道
	@return 返回下游的 ILane 接口
	*/
	virtual ILane* toLane() = 0;

};

#endif
