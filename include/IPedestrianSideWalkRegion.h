/******************************************************************************
* 行人 sidewalk 面域接口，由TESS NG实现，用于管理 sidewalk 的属性和行为
*******************************************************************************/

#pragma once

#include "IPedestrianRegion.h"

#include <QGraphicsPathItem>

class IHelpModifyPedestrianRegionSizeDotItem;

class IPedestrianSideWalkRegion : public QGraphicsPathItem, virtual public IPedestrianRegion
{
public:
    virtual ~IPedestrianSideWalkRegion() = default;

    /*
    获取 sidewalk 宽度
    @return 返回宽度值
    */
    virtual qreal getWidth() const = 0;

    /*
    设置 sidewalk 宽度
    @param width 宽度值
    */
    virtual void setWidth(qreal width) = 0;

    /*
    获取 sidewalk 顶点
    即初始折线顶点，是当前面域的子项
    @return 返回顶点接口指针列表
    */
    virtual QList<IHelpModifyPedestrianRegionSizeDotItem*> getVetexs() const = 0;

    /*
    获取 sidewalk 贝塞尔曲线控制点P1
    是当前面域的子项
    @return 返回控制点接口指针列表
    */
    virtual QList<IHelpModifyPedestrianRegionSizeDotItem*> getControl1Vetexs() const = 0;

    /*
    获取 sidewalk 贝塞尔曲线控制点P2
    是当前面域的子项
    @return 返回控制点接口指针列表
    */
    virtual QList<IHelpModifyPedestrianRegionSizeDotItem*> getControl2Vetexs() const = 0;

    /*
    获取候选顶点
    是当前面域的子项
    @return 返回候选顶点接口指针列表
    */
    virtual QList<IHelpModifyPedestrianRegionSizeDotItem*> getCandidateVetexs() const = 0;

    /*
    删除第index个顶点
    @param index 顶点索引
    */
    virtual void removeVetex(int index) = 0;

    /*
    在第index个位置插入顶点
    @param pos 初始位置坐标
    @param index 插入位置索引
    */
    virtual void insertVetex(QPointF pos, int index) = 0;
};
