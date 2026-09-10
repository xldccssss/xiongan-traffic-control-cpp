/********************************************************************************
* 公交站点接口，由TESS NG实现，用户可以借此访问公交站点对象方法，获取公交线路一些基本信息
*********************************************************************************/

#ifndef __IBusStation__
#define __IBusStation__

#include "tessinterfaces_global.h"

#include "IBusStationLine.h"
#include "UnitChange.h"
#include <QObject>
#include <QList>
#include <QPolygonF>

class ILink;
class ILane;
class IBusLine;

class TESSINTERFACES_EXPORT IBusStation
{
public:
	virtual ~IBusStation() = default;

	/*
	获取公交站点ID
	@return 返回公交站点的唯一标识ID
	*/
	virtual long id();

	/*
	获取公交站点名称
	@return 返回公交站点名称
	*/
	virtual QString name();

	/*
	获取公交站点所在车道序号
	@return 返回车道序号
	*/
	virtual int laneNumber();

	/*
	获取公交站点起始位置距路段起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距起点的距离
	*/
	virtual qreal distance(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取公交站点位置X坐标
	@param unit 单位类型，默认为默认单位
	@return 返回X坐标值
	*/
	virtual qreal x(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取公交站点位置Y坐标
	@param unit 单位类型，默认为默认单位
	@return 返回Y坐标值
	*/
	virtual qreal y(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取公交站点长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回站点长度
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取站点类型
	@return 返回站点类型：1表示路边式，2表示港湾式
	*/
	virtual int stationType();

	/*
	获取公交站点所在路段
	@return 返回站点所在的ILink接口
	*/
	virtual ILink* link();

	/*
	获取公交站点所在车道
	@return 返回站点所在的ILane接口
	*/
	virtual ILane* lane();

	/*
	设置站点名称
	@param name 站点名称
	*/
	virtual void setName(QString name);

	/*
	设置站点起始点距车道起点的距离
	@param dist 距车道起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setDistToStart(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置站点长度
	@param length 站点长度
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void setLength(qreal length, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置站点类型
	@param type 站点类型：1表示路侧式，2表示港湾式
	*/
	virtual void setType(int type);

	/*
	获取公交站点的多边形轮廓
	@return 返回站点的多边形轮廓
	*/
	virtual QPolygonF polygon();

	/*
	获取公交站点的方向线
	方向线是从起点到终点的线段
	@return 返回方向线（QLineF）
	*/
	virtual QLineF directionLine();
};

#endif