/******************************************************************************
* 信号相位接口，由TESS NG实现，用于管理信号相位的灯色配置及相关属性
*******************************************************************************/

#ifndef __ISignalPhase__
#define __ISignalPhase__

#include "tessinterfaces_global.h"

#include "Plugin/_datastruct.h"

#include <QObject>

class ISignalLamp;
class ISignalPlan;

class TESSINTERFACES_EXPORT ISignalPhase
{
public:
    virtual ~ISignalPhase() = default;

    /*
    获取相位ID
    @return 返回相位ID
    */
    virtual long id();

    /*
    获取相位名称
    @return 返回相位名称
    */
    virtual QString phaseName();

    /*
    获取相位灯色列表
    @return 返回灯色间隔列表
    */
    virtual QList<Online::ColorInterval> listColor();

    /*
    设置灯色列表
    @param lColor 灯色间隔列表
    */
    virtual void setColorList(QList<Online::ColorInterval> lColor);

    /*
    获取周期
    @return 返回周期时间（秒）
    */
    virtual int cycleTime();

    /*
    获取当前灯色
    @return 返回当前灯色枚举值
    */
    virtual Online::SignalPhaseColor phaseColor();

    /*
    获取信控方案
    @return 返回信控方案接口指针
    */
    virtual ISignalPlan* signalPlan();

    /*
    获取信号灯列表
    @return 返回信号灯接口指针列表
    */
    virtual QList<ISignalLamp*> signalLamps();

    /*
    获取信号灯ID组
    @return 返回信号灯ID组合字符串
    */
    virtual QString signalLampIds();

    /*
    设置相位名称
    @param name 相位名称
    */
    virtual void setPhaseName(QString name);
};

#endif
