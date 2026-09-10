/********************************************************************************
* 连接段接口，由TESS NG实现，用户可以借此调用接口方法，获取连接段一些基本属性
*********************************************************************************/

#ifndef __IConnector__
#define __IConnector__

#include "tessinterfaces_global.h"

#include "ISection.h"

#include <QJsonObject>
#include <QPolygonF>

class ILink;
class ILaneConnector;

class TESSINTERFACES_EXPORT IConnector : public ISection
{
public:
	virtual ~IConnector() = default;

	//============继承自ISection============

	/*
	获取Section ID
	由于当前是连接段，sectionID = 连接段ID + 10000000
	@return 返回Section ID
	*/
	virtual long sectionId() = 0;

	/*
	获取连接段ID
	@return 返回连接段的唯一标识ID
	*/
	virtual long id() = 0;

	/*
	获取连接段类型
	@return 返回类型枚举值（GLinkType 或 GConnectorType）
	*/
	virtual int gtype() = 0;

	/*
	获取连接段长度
	@param unit 单位类型，默认为默认单位
	@return 返回连接段长度
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取连接段名称
	@return 返回连接段名称
	*/
	virtual QString name() = 0;

	/*
	设置连接段名称
	@param name 连接段名称
	*/
	virtual void setName(QString name) = 0;

	/*
	获取连接段高程值
	@param unit 单位类型，默认为默认单位
	@return 返回高程值
	*/
	virtual qreal v3z(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取连接段上所有车道对象的列表
	@return 返回包含所有ILaneObject的列表
	*/
	virtual QList<ILaneObject*> laneObjects() = 0;

	/*
	根据ID获取上游Section
	如果当前是路段，id为0时返回空指针，否则返回上游指定ID的连接段；
	如果当前是连接段，id为0时返回上游路段，否则返回空指针
	@param id 连接段ID，默认为0
	@return 返回上游ISection接口，若无则返回空指针
	*/
	virtual ISection* fromSection(long id = 0) = 0;

	/*
	根据ID获取下游Section
	如果当前是路段，id为0时返回空指针，否则返回下游指定ID的连接段；
	如果当前是连接段，id为0时返回下游路段，否则返回空指针
	@param id 连接段ID，默认为0
	@return 返回下游ISection接口，若无则返回空指针
	*/
	virtual ISection* toSection(long id = 0) = 0;

	/*
	设置连接段的其他属性
	@param otherAttr 包含连接段其他属性的JSON对象
	*/
	virtual void setOtherAttr(QJsonObject otherAttr) = 0;

	/*
	获取连接段的多边形轮廓
	@return 返回连接段的多边形轮廓
	*/
	virtual QPolygonF polygon() = 0;


	//==========连接段特有属性============

	/*
	获取连接段高程
	@param unit 单位类型，默认为默认单位
	@return 返回高程值
	*/
	virtual qreal z(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取上游路段
	@return 返回上游ILink接口
	*/
	virtual ILink* fromLink() = 0;

	/*
	获取下游路段
	@return 返回下游ILink接口
	*/
	virtual ILink* toLink() = 0;

	/*
	获取连接段最高限速
	@param unit 单位类型，默认为默认单位（km/h）
	@return 返回最高限速值
	*/
	virtual qreal limitSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取连接段最低限速
	@param unit 单位类型，默认为默认单位（km/h）
	@return 返回最低限速值
	*/
	virtual qreal minSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取车道连接列表
	@return 返回所有ILaneConnector接口的列表
	*/
	virtual QList<ILaneConnector*> laneConnectors() = 0;

	/*
	设置车道连接的其他属性
	@param lAttrs 车道连接属性列表，顺序与车道连接顺序对应
	*/
	virtual void setLaneConnectorOtherAtrrs(QList<QJsonObject> lAttrs) = 0;

};

#endif