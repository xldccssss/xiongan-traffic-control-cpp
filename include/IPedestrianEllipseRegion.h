/******************************************************************************
* 行人椭圆面域接口，由TESS NG实现，用于表示椭圆形状的行人面域
*******************************************************************************/

#pragma once

#include "IPedestrianRegion.h"

#include <QGraphicsEllipseItem>

class IPedestrianEllipseRegion : public QGraphicsEllipseItem, virtual public IPedestrianRegion, virtual public IObstacleRegion, virtual public IPassengerRegion
{
public:
    virtual ~IPedestrianEllipseRegion() = default;
};
