/******************************************************************************
* 行人接口，由TESS NG实现，用户可以借此调用接口方法，获取行人基本属性和动态数据
*******************************************************************************/

#pragma once

#include "tessinterfaces_global.h"

class IPedestrianRegion;
class IPedestrianQueue;

class IPedestrian
{
public:
    virtual ~IPedestrian() = default;

    /*
    获取行人ID
    @return 返回行人ID
    */
    virtual long getId() const = 0;

    /*
    获取行人半径
    @return 返回半径值（米）
    */
    virtual qreal getRadius() const = 0;

    /*
    获取行人质量
    @return 返回质量值（千克）
    */
    virtual qreal getWeight() const = 0;

    /*
    获取行人颜色
    @return 返回颜色RGB值，格式如"#EE0000"
    */
    virtual QString getColor() const = 0;

    /*
    获取当前位置
    @return 返回当前位置坐标（米）
    */
    virtual QPointF getPos() const = 0;

    /*
    获取当前角度
    Qt坐标系下，x轴正方向为0，逆时针为正
    @return 返回角度值
    */
    virtual qreal getAngle() const = 0;

    /*
    获取当前二维方向向量
    @return 返回方向向量
    */
    virtual QVector2D getDirection() const = 0;

    /*
    获取当前高程
    @return 返回高程值（米）
    */
    virtual qreal getElevation() const = 0;

    /*
    获取当前速度
    @return 返回速度向量（米/秒）
    */
    virtual QVector2D getSpeed() const = 0;

    /*
    获取期望速度
    @return 返回期望速度值（米/秒）
    */
    virtual qreal getDesiredSpeed() const = 0;

    /*
    获取最大速度限制
    @return 返回最大速度值（米/秒）
    */
    virtual qreal getMaxSpeed() const = 0;

    /*
    获取当前加速度
    @return 返回加速度向量（米/秒²）
    */
    virtual QVector2D getAcce() const = 0;

    /*
    获取最大加速度限制
    @return 返回最大加速度值（米/秒²）
    */
    virtual qreal getMaxAcce() const = 0;

    /*
    获取欧拉角
    @return 返回欧拉角向量
    */
    virtual QVector3D getEuler() const = 0;

    /*
    获取速度欧拉角
    @return 返回速度欧拉角向量
    */
    virtual QVector3D getSpeedEuler() const = 0;

    /*
    获取墙壁方向单位向量
    @return 返回墙壁方向单位向量
    */
    virtual QVector2D getWallFDirection() const = 0;

    /*
    获取当前所在面域
    @return 返回面域接口指针
    */
    virtual IPedestrianRegion* getRegion() const = 0;

    /*
    获取行人类型ID
    @return 返回行人类型ID
    */
    virtual long getPedestrianTypeId() const = 0;

    /*
    设置行人轨迹
    @param path 行人轨迹路径
    */
    virtual void setPedestrianTrajectory(QPainterPath path) = 0;

    /*
    移动到指定位置
    @param pos 目标位置（米）
    @param goalPoint 朝向目标点（米）
    */
    virtual void moveTo(QPointF pos, QPointF goalPoint) = 0;

    /*
    设置临时目标位置
    @param pos 临时目标位置（米）
    */
    virtual void setTempTargetPos(QPointF pos) = 0;

    /*
    获取目标距离
    @return 返回目标距离（米）
    */
    virtual qreal getTargetDist() const = 0;

    /*
    获取队列通过状态
    0表示还未接近排队点，1表示到达排队点，2正在向排队点移动，3表示正在排队
    @return 返回队列通过状态
    */
    virtual int getQueuePassStatus() const = 0;

    /*
    设置队列通过状态
    @param status 队列通过状态
    */
    virtual void setQueuePassStatus(int status) = 0;

    /*
    获取当前正在通过的队列
    @return 返回队列接口指针
    */
    virtual IPedestrianQueue* getCurQueue() const = 0;

    /*
    设置当前正在通过的队列
    @param queue 队列接口指针
    */
    virtual void setCurQueue(IPedestrianQueue* queue) = 0;

    /*
    停止仿真，会在下一个仿真批次移除当前行人，释放资源
    */
    virtual void stop() = 0;
};
