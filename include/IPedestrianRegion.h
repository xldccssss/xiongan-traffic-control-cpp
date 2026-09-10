/******************************************************************************
* 行人面域接口，由TESS NG实现，定义行人面域的基本属性和行为
*******************************************************************************/

#pragma once

#include "IObstacleRegion.h"
#include "IPassengerRegion.h"
#include "IPedestrianPathRegionBase.h"

class IPedestrianRegion : virtual public IPedestrianPathRegionBase
{
public:
    virtual ~IPedestrianRegion() = default;

    /*
    获取面域ID
    @return 返回面域ID
    */
    virtual long getId() const = 0;

    /*
    获取面域名称
    @return 返回面域名称
    */
    virtual QString getName() const = 0;

    /*
    设置面域名称
    @param name 面域名称
    */
    virtual void setName(QString name) = 0;

    /*
    获取面域颜色
    @return 返回面域颜色
    */
    virtual QColor getRegionColor() const = 0;

    /*
    设置面域颜色
    @param color 面域颜色
    */
    virtual void setRegionColor(QColor color) = 0;

    /*
    获取面域位置
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回面域位置坐标
    */
    virtual QPointF getPosition(UnitOfMeasure unit = UnitOfMeasure::Default) const = 0;

    /*
    设置面域位置
    @param scenePos 场景坐标系下的位置，默认单位为像素
    @param unit 单位类型，默认为默认单位（像素）
    */
    virtual void setPosition(QPointF scenePos, UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取面域类型
    @return 返回面域类型
    */
    virtual int getGType() const = 0;

    /*
    获取期望速度系数
    @return 返回期望速度系数
    */
    virtual qreal getExpectSpeedFactor() const = 0;

    /*
    设置期望速度系数
    @param val 期望速度系数
    */
    virtual void setExpectSpeedFactor(qreal val) = 0;

    /*
    获取面域高程
    @return 返回高程值
    */
    virtual qreal getElevation() const = 0;

    /*
    设置面域高程
    @param elevation 高程值
    */
    virtual void setElevation(qreal elevation) = 0;

    /*
    获取面域多边形
    @param sceneCoords 是否使用场景坐标，默认为false
    @return 返回面域多边形
    */
    virtual QPolygonF getPolygon(bool sceneCoords = false) const = 0;

    /*
    获取面域所在图层ID
    @return 返回图层ID
    */
    virtual long getLayerId() const = 0;

    /*
    设置面域所在图层
    如果图层ID非法，则不做任何改变
    @param id 图层ID
    */
    virtual void setLayerId(long id) = 0;
};
