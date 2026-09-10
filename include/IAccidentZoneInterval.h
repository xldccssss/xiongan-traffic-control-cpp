/********************************************************************************
* 事故区时段接口，由TESS NG实现，用户可以借此调用接口方法，获取事故区时段的基本属性
*********************************************************************************/

#ifndef __IAccidentZoneInterval__
#define __IAccidentZoneInterval__

#include <QObject>
#include <QList>
#include "tessinterfaces_global.h"
#include "UnitChange.h"

class TESSINTERFACES_EXPORT IAccidentZoneInterval
{
public:
    virtual ~IAccidentZoneInterval() = default;

    /*
    获取事故区时段ID
    @return 返回事故区时段的唯一标识ID
    */
    virtual long intervalId() = 0;

    /*
    获取所属事故区ID
    @return 返回所属事故区的ID
    */
    virtual long accidentZoneId() = 0;

    /*
    获取事故区时段开始时间
    @return 返回时段开始时间戳
    */
    virtual long startTime() = 0;

    /*
    获取事故区时段结束时间
    @return 返回时段结束时间戳
    */
    virtual long endTime() = 0;

    /*
    获取事故区在该时段的长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回事故区长度
    */
    virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取事故区在该时段距起点的距离
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回距起点的距离
    */
    virtual qreal location(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取事故区在该时段的限速
    @param unit 单位类型，默认为默认单位（km/h）
    @return 返回限速值
    */
    virtual qreal limitedSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取事故区在该时段的控制距离
    车辆距离事故区起点该距离内，强制变道
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回控制距离
    */
    virtual qreal controlLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取事故区在该时段的占用车道序号列表
    @return 返回占用车道的序号列表
    */
    virtual QList<int> laneNumbers() = 0;
};

#endif