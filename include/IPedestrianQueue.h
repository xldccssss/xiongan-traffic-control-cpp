/******************************************************************************
* 行人队列接口，由TESS NG实现，用于管理行人队列的行为
*******************************************************************************/

#pragma once

class IPedestrian;

class IPedestrianQueue
{
public:
    virtual ~IPedestrianQueue() = default;

    /*
    移动队列
    @param stepSecondTime 步长时间（秒）
    */
    virtual void moveQueue(qreal stepSecondTime) = 0;

    /*
    添加行人到队列
    @param pedestrian 行人接口指针
    */
    virtual void addPedestrian(IPedestrian* pedestrian) = 0;

    /*
    获取排队点位置
    @return 返回排队点位置坐标（米）
    */
    virtual QPointF getEnqueuePos() const = 0;

    /*
    获取队列起始位置
    @return 返回队列起始位置坐标（米）
    */
    virtual QPointF getStartPos() const = 0;

    /*
    获取队列结束位置
    @return 返回队列结束位置坐标（米）
    */
    virtual QPointF getEndPos() const = 0;

    /*
    获取队列线中间位置
    @return 返回队列线中间位置坐标（米）
    */
    virtual QPointF getMiddlePos() const = 0;

    /*
    设置队列路径
    @param path 队列路径
    */
    virtual void setQueuePath(const QPainterPath& path) = 0;
};
