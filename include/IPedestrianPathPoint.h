/******************************************************************************
* 行人路径点接口，由TESS NG实现
* 通常作为面域的子项使用，位置坐标为所属面域坐标系下的点位
*******************************************************************************/

#pragma once

#include <QGraphicsPathItem>

#include "tessinterfaces_global.h"
#include "UnitChange.h"

class TESSINTERFACES_EXPORT IPedestrianPathPoint: public QGraphicsPathItem
{
public:
    /*
    构造函数
    @param parent 父图形项指针，默认为nullptr
    */
    IPedestrianPathPoint(QGraphicsItem* parent = nullptr);

    virtual ~IPedestrianPathPoint() = default;

    /*
    获取行人路径点ID
    @return 返回路径点ID
    */
    virtual long getId() const = 0;

    /*
    获取行人路径点场景坐标系下的位置
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回场景坐标系下的位置
    */
    virtual QPointF getScenePos(UnitOfMeasure unit = UnitOfMeasure::Default) const = 0;

    /*
    获取行人路径点的半径
    @return 返回半径值（米）
    */
    virtual qreal getRadius() const = 0;

    /*
    验证新位置是否合法
    可能会导致面域切换
    @return 位置合法返回true，否则返回false
    */
    virtual bool verifyNewPosition() = 0;
};
