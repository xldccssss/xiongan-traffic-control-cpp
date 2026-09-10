/******************************************************************************
* 信控方案接口，由TESS NG实现，用于管理信控方案的周期配置和相位管理
*******************************************************************************/

#pragma once

#include <QObject>

#include "tessinterfaces_global.h"

#include "Plugin/_datastruct.h"

class ISignalPhase;

class TESSINTERFACES_EXPORT ISignalPlan : public QObject
{
    Q_OBJECT

public:
    virtual ~ISignalPlan() = default;

    /*
    获取信控方案ID
    @return 返回ID
    */
    virtual long id();

    /*
    获取信控方案名称
    @return 返回方案名称
    */
    virtual QString name();

    /*
    获取信号机名称
    @return 返回信号机名称
    */
    virtual QString trafficName();

    /*
    获取信号周期
    @return 返回周期时间（秒）
    */
    virtual int cycleTime();

    /*
    获取起始时间
    @return 返回起始时间（秒）
    */
    virtual long fromTime();

    /*
    获取结束时间
    @return 返回结束时间（秒）
    */
    virtual long toTime();

    /*
    获取相位差
    @return 返回相位差值（秒）
    */
    virtual int phaseDifference();

    /*
    获取相位列表
    @return 返回相位接口指针列表
    */
    virtual QList<ISignalPhase*> phases();

    /*
    设置信号灯组/信控方案名称
    @param name 方案名称
    */
    virtual void setName(QString name);

    /*
    设置信号周期
    @param period 周期时间（秒）
    */
    virtual void setCycleTime(int period);

    /*
    设置起始时间
    @param time 起始时间（秒）
    */
    virtual void setFromTime(long time);

    /*
    设置结束时间
    @param time 结束时间（秒）
    */
    virtual void setToTime(long time);

    /*
    设置相位差
    @param time 相位差时间（秒）
    */
    virtual void setPhaseDifference(long time);
};
