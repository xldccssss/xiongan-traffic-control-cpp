/********************************************************************************
* 人行横道红绿灯接口
* 通常作为人行横道面域的子项使用，位置坐标为人行横道坐标系下的点位
* @author 孙文光
*********************************************************************************/

#pragma once

#include "isignallamp.h"
#include "tessinterfaces_global.h"
#include <QGraphicsPathItem>

class IPedestrianCrossWalkRegion;

class TESSINTERFACES_EXPORT ICrosswalkSignalLamp : public ISignalLamp, public QGraphicsPathItem
{
public:
    /*
    构造函数
    @param parent 父图形项
    */
    ICrosswalkSignalLamp(QGraphicsItem* parent);

    /*
    析构函数
    */
    virtual ~ICrosswalkSignalLamp() = default;

    /*
    获取所属人行横道
    @return 返回所属的IPedestrianCrossWalkRegion接口
    */
    virtual IPedestrianCrossWalkRegion* getICrossWalk() const = 0;
};