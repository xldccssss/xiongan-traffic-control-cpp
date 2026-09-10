/******************************************************************************
* 行人扇形面域接口，由TESS NG实现，用于表示扇形形状的行人面域
*******************************************************************************/

#pragma once

#include "IPedestrianRegion.h"

#include <QGraphicsPathItem>

class IPedestrianFanShapeRegion : public QGraphicsPathItem, virtual public IPedestrianRegion, virtual public IObstacleRegion, virtual public IPassengerRegion
{
public:
    virtual ~IPedestrianFanShapeRegion() = default;

    /*
    获取内半径
    @return 返回内半径值（米）
    */
    virtual qreal getInnerRadius() const = 0;

    /*
    获取外半径
    @return 返回外半径值（米）
    */
    virtual qreal getOuterRadius() const = 0;

    /*
    获取起始角度
    Qt坐标系下，x轴正方向为0，逆时针为正
    @return 返回起始角度值（度）
    */
    virtual qreal getStartAngle() const = 0;

    /*
    获取扫过角度
    Qt坐标系下，x轴正方向为0，逆时针为正
    @return 返回扫过角度值（度）
    */
    virtual qreal getSweepAngle() const = 0;
};
