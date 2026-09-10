/******************************************************************************
* 行人多边形面域接口，由TESS NG实现，用于表示多边形形状的行人面域
*******************************************************************************/

#pragma once

#include "IPedestrianRegion.h"

#include <QGraphicsPolygonItem>

class IPedestrianPolygonRegion : public QGraphicsPolygonItem, virtual public IPedestrianRegion, virtual public IObstacleRegion, virtual public IPassengerRegion
{
public:
    virtual ~IPedestrianPolygonRegion() = default;
};
