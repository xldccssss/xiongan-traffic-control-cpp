/******************************************************************************
* 行人三角形面域接口，由TESS NG实现，用于表示三角形形状的行人面域
*******************************************************************************/

#pragma once

#include "IPedestrianRegion.h"

#include <QGraphicsPolygonItem>

class IPedestrianTriangleRegion : public QGraphicsPolygonItem, virtual public IPedestrianRegion, virtual public IObstacleRegion, virtual public IPassengerRegion
{
public:
    virtual ~IPedestrianTriangleRegion() = default;
};
