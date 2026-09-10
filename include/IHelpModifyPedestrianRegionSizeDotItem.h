/********************************************************************************
* 人行区域大小修改辅助点接口
* 用于辅助修改人行区域大小的控制点
*********************************************************************************/

#pragma once

#include <QObject>
#include <QGraphicsEllipseItem>
#include <QPointF>

class IHelpModifyPedestrianRegionSizeDotItem : public QObject, public QGraphicsEllipseItem
{
public:
    virtual ~IHelpModifyPedestrianRegionSizeDotItem() = default;

    /*
    设置位置
    @param pos 坐标点（所属面域的局部坐标系坐标）
    */
    virtual void setPosition(QPointF pos) = 0;

    /*
    获取位置
    @param useSceneCoords 是否返回基于场景坐标系的坐标，默认为false
    @return 返回坐标点，默认返回所属面域的局部坐标系坐标
    */
    virtual QPointF getPosition(bool useSceneCoords = false) = 0;
};