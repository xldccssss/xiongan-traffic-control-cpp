/******************************************************************************
* 行人路径面域基础接口，由TESS NG实现
* 定义行人面域的基础属性和行为
*******************************************************************************/

#pragma once

#include "UnitChange.h"

class IPedestrianPathRegionBase
{
public:
    virtual ~IPedestrianPathRegionBase() = default;

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
    获取是否进行面域评价
    @return 进行评价返回true，否则返回false
    */
    virtual bool isEvaluationRegion() const = 0;

    /*
    设置是否进行面域评价
    @param b 是否进行评价
    */
    virtual void setEvaluationRegion(bool b) = 0;
};
