/********************************************************************************
* 路段接口，由TESS NG实现，用户可以借此调用接口方法，获取路段一些基本属性
*********************************************************************************/

#ifndef __ILink__
#define __ILink__

#include "tessinterfaces_global.h"

#include "ISection.h"
#include "ILane.h"
#include "UnitChange.h"

#include <QVector3D>
#include <QJsonObject>

class IRouting;
class IConnector;

class TESSINTERFACES_EXPORT  ILink : public ISection
{
public:
	virtual ~ILink() = default;
	//============继承自ISection============
	/*
	获取Section ID，由于当前是路段，sectionID即为路段的ID
	@return 返回路段的ID
	*/
	virtual long sectionId() = 0;
	/*
	获取路段ID
	@return 返回路段的唯一标识ID
	*/
	virtual long id() = 0;
	/*
	获取路段类型（GLinkType）
	@return 返回路段类型枚举值
	*/
	virtual int gtype() = 0;
	/*
	获取Section名称
	@return 返回路段的名称
	*/
	virtual QString name() = 0;
	/*
	设置Section名称
	@param name 路段的新名称
	*/
	virtual void setName(QString name) = 0;
	/*
	获取路段高程值
	@param unit 单位类型，默认为默认单位
	@return 返回路段的高程值
	*/
	virtual qreal v3z(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段长度
	@param unit 单位类型，默认为默认单位
	@return 返回路段的长度
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段上所有车道对象的列表
	@return 返回包含所有ILaneObject的列表
	*/
	virtual QList<ILaneObject*> laneObjects() = 0;
	/*
	根据ID获取上游Section
	如果当前是路段，id为0时返回空指针，否则返回上游指定ID的连接段；
	如果当前是连接段，id为0时返回上游路段，否则返回空指针
	@param id 连接段ID，默认为0
	@return 返回上游Section接口，若无则返回空指针
	*/
	virtual ISection* fromSection(long id = 0) = 0;
	/*
	根据ID获取下游Section
	如果当前是路段，id为0时返回空指针，否则返回下游指定ID的连接段；
	如果当前是连接段，id为0时返回下游路段，否则返回空指针
	@param id 连接段ID，默认为0
	@return 返回下游Section接口，若无则返回空指针
	*/
	virtual ISection* toSection(long id = 0) = 0;
	/*
	获取路段的其他属性（JSON格式）
	@return 返回包含路段其他属性的JSON对象
	*/
	virtual QJsonObject otherAttr() = 0;
	/*
	设置路段的其他属性
	@param otherAttr 包含路段其他属性的JSON对象
	*/
	virtual void setOtherAttr(QJsonObject otherAttr) = 0;
	/*
	获取路段的多边形轮廓
	@return 返回路段的多边形轮廓
	*/
	virtual QPolygonF polygon() = 0;


	//============路段特有属性============
	/*
	获取路段宽度
	@param unit 单位类型，默认为默认单位
	@return 返回路段的宽度
	*/
	virtual qreal width(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段高程
	@param unit 单位类型，默认为默认单位
	@return 返回路段的高程值
	*/
	virtual qreal z(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段的车道数量
	@return 返回车道总数
	*/
	virtual int laneCount() = 0;
	/*
	获取路段的最高限速
	@param unit 单位类型，默认为默认单位
	@return 返回最高限速值
	*/
	virtual double limitSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取路段类型
	@return 返回路段类型字符串
	*/
	virtual QString linkType() = 0;
	/*
	设置路段的最高限速
	@param speed 最高限速值
	@param unit 单位类型，默认为默认单位
	*/
	virtual void setLimitSpeed(qreal speed, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	设置路段类型
	@param type 路段类型字符串
	*/
	virtual void setType(QString type) = 0;
	/*
	获取路段的最低限速
	@param unit 单位类型，默认为默认单位
	@return 返回最低限速值
	*/
	virtual qreal minSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	设置路段的最低限速
	@param speed 最低限速值
	@param unit 单位类型，默认为默认单位
	*/
	virtual void setMinSpeed(qreal speed, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段所有车道的接口列表
	@return 返回包含所有车道接口的列表
	*/
	virtual QList<ILane*> lanes() = 0;
	/*
	获取上游连接段列表
	@return 返回所有上游连接段的接口列表
	*/
	virtual QList<IConnector*> fromConnectors() = 0;
	/*
	获取下游连接段列表
	@return 返回所有下游连接段的接口列表
	*/
	virtual QList<IConnector*> toConnectors() = 0;
	/*
	获取路段中心线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> centerBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段左侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> leftBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段右侧线断点集（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的二维坐标列表
	*/
	virtual QList<QPointF> rightBreakPoints(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段中心线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回中心线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> centerBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段左侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回左侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> leftBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	获取路段右侧线断点集（三维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回右侧线所有断点的三维坐标列表
	*/
	virtual QList<QVector3D> rightBreakPoint3Ds(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
	/*
	设置各车道的类型
	@param lType 车道类型列表，顺序与车道顺序对应
	*/
	virtual void setLaneTypes(QList<QString> lType) = 0;
	/*
	设置各车道的其它属性
	@param lAttrs 车道属性列表，顺序与车道顺序对应
	*/
	virtual void setLaneOtherAtrrs(QList<QJsonObject> lAttrs) = 0;
	/*
	计算中心线上指定点到起点的距离
	@param p 中心线上的点（二维坐标）
	@param unit 单位类型，默认为默认单位
	@return 返回该点到路段起点的距离
	*/
	virtual qreal distToStartPoint(const QPointF p, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
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

};

#endif