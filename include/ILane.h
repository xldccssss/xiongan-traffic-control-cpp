/********************************************************************************
* 车道接口，由TESS NG实现，用户可以借此调用接口方法，获取车道一些基本属性
*********************************************************************************/

#ifndef __ILane__
#define __ILane__

#include "tessinterfaces_global.h"
#include "ILaneObject.h"
#include "UnitChange.h"

#include <QObject>
#include <QList>
#include <QPointF>

class ILink;
class ILane;
class ILaneObject;
class _LaneLimitChange;
class ILaneConnector;
class SimulatorBatchCommands;

class TESSINTERFACES_EXPORT ILane : public ILaneObject
{
public:
	// 车道下一批次控制代理，仅保存既有命令键所需的路段与车道信息。
	class Step
	{
	public:
		TESSINTERFACES_EXPORT void setSpeedLimit(qreal speedLimitKph);

	private:
		Step() = default;

		long mLinkId = 0;
		int mLaneNumber = 0;

		friend class ILane;
	};

	ILane();
	virtual ~ILane();
	//============继承自ILaneObject的方法============
	/*
	获取车道类型（GLaneType）
	@return 返回车道类型枚举值
	*/
	virtual int gtype() = 0;
	/*
	获取所属Section
	@return 返回车道所属的ISection接口
	*/
	virtual ISection* section() = 0;
	/*
	获取车道ID
	@return 返回车道的唯一标识ID
	*/
	virtual long id() = 0;
	/*
	获取车道长度
	@param unit 单位类型，默认为默认单位
	@return 返回车道的长度
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	根据ID获取上游LaneObject
	id为0时返回空指针，否则返回上游指定ID的"车道连接"
	@param id 车道连接ID，默认为0
	@return 返回上游ILaneObject接口，若无则返回空指针
	*/
	virtual ILaneObject* fromLaneObject(long id = 0) = 0;
	/*
	根据ID获取下游LaneObject
	id为0时返回空指针，否则返回下游指定ID的"车道连接"
	@param id 车道连接ID，默认为0
	@return 返回下游ILaneObject接口，若无则返回空指针
	*/
	virtual ILaneObject* toLaneObject(long id = 0) = 0;
	/*
	获取车道中心线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> centerBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道左侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> leftBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道右侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> rightBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道中心线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> centerBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道左侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> leftBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道右侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> rightBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道左侧部分断点集（三维坐标）
	@param fromPoint 起点（中心线上的点）
	@param toPoint 终点（中心线上的点）
	@param unit 单位类型，默认为默认单位
	@return 返回指定范围内左侧线断点的三维坐标列表
	*/
	virtual QList<QVector3D> leftBreak3DsPartly(QPointF fromPoint, QPointF toPoint, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道右侧部分断点集（三维坐标）
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
	@return 返回该点到车道起点的距离
	*/
	virtual qreal distToStartPoint(const QPointF p, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	计算中心线上指定点到起点的距离（带分段序号）
	附加条件是该点所在车道上的分段序号
	@param p 中心线上的点（二维坐标）
	@param segmIndex 分段序号，默认为0
	@param bOnCentLine 点是否在中心线上，默认为true
	@param unit 单位类型，默认为默认单位
	@return 返回该点到车道起点的距离
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
	设置车道的其他属性
	@param attr 包含车道其他属性的JSON对象
	*/
	virtual void setOtherAttr(QJsonObject attr) = 0;
	/*
	获取车道的多边形轮廓
	@return 返回车道的多边形轮廓
	*/
	virtual QPolygonF polygon() = 0;

	//============车道特有方法============
	/*
	获取车道所在路段
	@return 返回车道所属的ILink接口
	*/
	virtual ILink* link() = 0;
	/*
	获取车道宽度
	@param unit 单位类型，默认为默认单位
	@return 返回车道的宽度
	*/
	virtual qreal width(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取车道序号
	序号从0开始，自外侧往内侧计数
	@return 返回车道的序号
	*/
	virtual int number() = 0;
	/*
	获取车道行为类型
	行为类型包括："机动车道"、"非机动车道"、"公交专用道"等
	@return 返回车道行为类型字符串
	*/
	virtual QString actionType() = 0;

	/*
	设置车道类型
	@param type 车道类型字符串
	*/
	virtual void setLaneType(QString type) = 0;

	/*
	判断是否为应急车道
	@return 如果是应急车道返回true，否则返回false
	*/
	virtual bool emergencyLane() = 0;
	/*
	设置是否为应急车道
	@param value true表示设置为应急车道，false表示取消应急车道
	*/
	virtual void setEmergencyLane(bool value) = 0;

	/*
	新增限制变道信息
	laneLimitChange ID设置为默认值，如果设置大于0需要自己判断唯一性避免保存出错
	@param laneLimitChange 限制变道信息对象
	@return 返回新增后的限制变道信息
	*/
	virtual _LaneLimitChange addLimitChange(const _LaneLimitChange& laneLimitChange) = 0;
	/*
	更新限制变道信息
	@param laneLimitChange 更新后的限制变道信息对象
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateLimitChange(const _LaneLimitChange& laneLimitChange) = 0;
	/*
	删除限制变道信息
	根据参数属性ID删除指定的限制变道
	@param limitChangeID 限制变道的ID
	@return 删除成功返回true，否则返回false
	*/
	virtual bool delLimitChange(long limitChangeID) = 0;
	/*
	查询限制变道信息列表
	@return 返回所有限制变道信息的列表
	*/
	virtual QList<_LaneLimitChange> limitChanges() = 0;
	/*
	获取限制变道断点向量
	@return 返回限制变道断点的向量列表
	*/
	virtual QVector<QList<QPointF>> getLimitChangeBreakPointVec() = 0;

	/*
	获取上游车道连接列表
	@return 返回所有上游ILaneConnector接口的列表
	*/
	virtual QList<ILaneConnector*> fromLaneConnectors() = 0;
	/*
	获取下游车道连接列表
	@return 返回所有下游ILaneConnector接口的列表
	*/
	virtual QList<ILaneConnector*> toLaneConnectors() = 0;

	/*
	获取下一批次控制代理。返回指针由当前车道持有，不能跨越车道生命周期保留。
	*/
	Step* step();

private:
	static SimulatorBatchCommands* currentStepCommands(const char* methodName);
	Step* mStep = nullptr;
};

#endif 
