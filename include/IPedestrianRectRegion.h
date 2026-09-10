/******************************************************************************
* 行人矩形面域接口，由TESS NG实现，用于表示矩形形状的行人面域
*******************************************************************************/

#pragma once

#include "IPedestrianRegion.h"

#include <QGraphicsRectItem>

class IPedestrianRectRegion : public QGraphicsRectItem, virtual public IPedestrianRegion, virtual public IObstacleRegion, virtual public IPassengerRegion
{
public:
    virtual ~IPedestrianRectRegion() = default;
};
