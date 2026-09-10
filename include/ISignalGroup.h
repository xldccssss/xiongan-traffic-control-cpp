/******************************************************************************
* 信号灯组接口，由TESS NG实现，用于管理信号灯组的基本属性和相位配置
*******************************************************************************/

#ifndef __ISignalGroup__
#define __ISignalGroup__

#include "tessinterfaces_global.h"

#include <QObject>

class ISignalPhase;

class TESSINTERFACES_EXPORT ISignalGroup
{
public:
    virtual ~ISignalGroup() = default;

    /*
    获取信号灯组ID
    @return 返回ID
    */
    virtual long id();

    /*
    获取灯组名
    @return 返回灯组名称
    */
    virtual QString groupName();

    /*
    获取信号周期
    @return 返回周期时间（秒）
    */
    virtual int periodTime();

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
    获取相位列表
    @return 返回相位接口指针列表
    */
    virtual QList<ISignalPhase*> phases();

    /*
    设置信号灯组名称
    @param name 灯组名称
    */
    virtual void setName(QString name);

    /*
    设置信号周期
    @param period 周期时间（秒）
    */
    virtual void setPeriodTime(int period);

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
};

#endif
