/******************************************************************************
* 人行横道面域接口，由TESS NG实现，用于管理人行横道的属性和行为
*******************************************************************************/

#pragma once

#include "IPedestrianRegion.h"

#include <QGraphicsPathItem>

class ICrosswalkSignalLamp;
class IHelpModifyPedestrianRegionSizeDotItem;

class IPedestrianCrossWalkRegion : public QGraphicsPathItem, virtual public IPedestrianRegion
{
public:
    virtual ~IPedestrianCrossWalkRegion() = default;

    /*
    获取人行横道的宽度
    @return 返回宽度值（米）
    */
    virtual qreal getWidth() const = 0;

    /*
    设置人行横道的宽度
    @param width 宽度值（米）
    */
    virtual void setWidth(qreal width) = 0;

    /*
    获取人行横道起点到终点的线段
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回场景坐标系下的线段
    */
    virtual QLineF getSceneLine(UnitOfMeasure unit = UnitOfMeasure::Default) const = 0;

    /*
    获取人行横道倾斜角度
    人行横道自身坐标系下，以垂直于起终点连线为0度，大于0度向左倾斜，小于0度向右倾斜
    @return 返回角度值（度）
    */
    virtual qreal getAngle() const = 0;

    /*
    设置人行横道倾斜角度
    人行横道自身坐标系下，以垂直于起终点连线为0度，大于0度向左倾斜，小于0度向右倾斜
    @param angle 角度值（度）
    */
    virtual void setAngle(qreal angle) = 0;

    /*
    获取红灯清尾速度系数
    @return 返回速度系数
    */
    virtual qreal getRedLightSpeedFactor() const = 0;

    /*
    设置红灯清尾速度系数
    @param factor 速度系数
    */
    virtual void setRedLightSpeedFactor(qreal factor) = 0;

    /*
    获取场景坐标系下从起点到终点的单位方向向量
    @return 返回单位方向向量
    */
    virtual QVector2D getUnitDirectionFromStartToEnd() const = 0;

    /*
    获取人行横道本身坐标系下从起点到终点的单位方向向量
    @return 返回单位方向向量
    */
    virtual QVector2D getLocalUnitDirectionFromStartToEnd() const = 0;

    /*
    获取起点控制点
    @return 返回控制点接口指针
    */
    virtual IHelpModifyPedestrianRegionSizeDotItem* getStartControlPoint() const = 0;

    /*
    获取终点控制点
    @return 返回控制点接口指针
    */
    virtual IHelpModifyPedestrianRegionSizeDotItem* getEndControlPoint() const = 0;

    /*
    获取左侧控制点
    @return 返回控制点接口指针
    */
    virtual IHelpModifyPedestrianRegionSizeDotItem* getLeftControlPoint() const = 0;

    /*
    获取右侧控制点
    @return 返回控制点接口指针
    */
    virtual IHelpModifyPedestrianRegionSizeDotItem* getRightControlPoint() const = 0;

    /*
    获取管控正向通行的信号灯
    是当前面域的子项
    @return 返回信号灯接口指针
    */
    virtual ICrosswalkSignalLamp* getPositiveDirectionSignalLamp() const = 0;

    /*
    获取管控反向通行的信号灯
    是当前面域的子项
    @return 返回信号灯接口指针
    */
    virtual ICrosswalkSignalLamp* getNegativeDirectionSignalLamp() const = 0;

    /*
    判断是否添加了管控正向通行的信号灯
    @return 已添加返回true，否则返回false
    */
    virtual bool isPositiveTrafficLightAdded() const = 0;

    /*
    判断是否添加了管控反向通行的信号灯
    @return 已添加返回true，否则返回false
    */
    virtual bool isReverseTrafficLightAdded() const = 0;

    /*
    获取斑马线白色区块的多边形
    @return 返回Qt场景坐标系下的一组多边形（米制单位）
    */
    virtual QList<QPolygonF> getWhiteBlockPolygons() const = 0;
};
