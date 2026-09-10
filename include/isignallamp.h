/******************************************************************************
* 信号灯接口，由TESS NG实现，用户可以借此调用接口方法，获取信号灯一些基本属性
*******************************************************************************/

#ifndef ISIGNALLAMP_H
#define ISIGNALLAMP_H

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QObject>
#include <QColor>
#include <QPolygonF>

class ISignalPhase;
class ILaneObject;
class ISignalPlan;
class SimulatorBatchCommands;

class TESSINTERFACES_EXPORT ISignalLamp
{
public:
    // 信号灯下一批次控制代理，仅保存信号灯 ID。
    class Step
    {
    public:
        TESSINTERFACES_EXPORT void setLampColor(const QString& color);

    private:
        Step() = default;

        long mSignalLampId = 0;

        friend class ISignalLamp;
    };

    ISignalLamp();
    virtual ~ISignalLamp();

    /*
    获取信号灯ID
    @return 返回信号灯ID
    */
    virtual long id();

    /*
    设置相位
    所设相位可以是其它信号灯组的相位
    @param pPhase 相位接口指针
    */
    virtual void setSignalPhase(ISignalPhase* pPhase);

    /*
    设置相位序号
    序号从1开始，如果num序号大于相位总数不进行设置
    @param num 相位序号
    */
    virtual void setPhaseNumber(int num);

    /*
    设置灯色
    colorStr为"红"、"绿"、"黄"、"灰"，或者"R"、"G"、"Y"、"gray"分别表示红绿黄灰
    @param colorStr 灯色字符串
    */
    virtual void setLampColor(QString colorStr);

    /*
    获取信号灯色
    "R"、"G"、"Y"、"gray"分别表示"红"、"绿"、"黄"、"灰"
    @return 返回灯色字符串
    */
    virtual QString color();

    /*
    获取信号灯名称
    @return 返回信号灯名称
    */
    virtual QString name();

    /*
    设置信号灯名称
    @param name 信号灯名称
    */
    virtual void setName(QString name);

    /*
    设置信号灯距车道起点（或"车道连接"起点）距离
    @param dist 距离值
    @param unit 单位类型，默认为默认单位（像素）
    */
    virtual void setDistToStart(qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

    /*
    获取信控方案
    @return 返回信控方案接口指针
    */
    virtual ISignalPlan* signalPlan();

    /*
    获取当前相位
    @return 返回相位接口指针
    */
    virtual ISignalPhase* signalPhase();

    /*
    获取相位列表
    @return 返回相位接口指针列表
    */
    virtual QList<ISignalPhase*> lsignalPhases();

    /*
    获取所在车道或车道连接
    @return 返回ILaneObject接口指针
    */
    virtual ILaneObject* laneObject();

    /*
    获取多边形轮廓
    @return 返回多边形
    */
    virtual QPolygonF polygon();

    /*
    获取角度
    @return 返回角度值
    */
    virtual qreal angle();

    /*
    获取下一批次控制代理。返回指针由当前信号灯持有，不能跨越信号灯生命周期保留。
    */
    Step* step();

private:
    static SimulatorBatchCommands* currentStepCommands(const char* methodName);
    Step* mStep = nullptr;
};

#endif // ISIGNALLAMP_H
