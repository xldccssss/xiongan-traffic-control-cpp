/******************************************************************************
* 施工区接口，由TESS NG实现，用于管理道路施工区的属性和管理相关参数
*******************************************************************************/

#ifndef __IRoadWorkZone__
#define __IRoadWorkZone__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"
#include "UnitChange.h"
#include <QObject>

class ILaneObject;

class TESSINTERFACES_EXPORT IRoadWorkZone
{
public:
    virtual ~IRoadWorkZone() = default;

    /*
    获取施工区ID
    @return 返回施工区ID
    */
    virtual long id() = 0;

    /*
    获取施工区名称
    @return 返回施工区名称
    */
    virtual QString name() = 0;

    /*
    获取距起点距离
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回距离值
    */
    virtual qreal location(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取施工区长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值
    */
    virtual qreal zoneLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取施工区限速（最大车速）
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回限速值（km/h）
    */
    virtual qreal limitSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取路段或连接段ID
    @return 返回Section ID
    */
    virtual long sectionId() = 0;

    /*
    获取Section名称
    @return 返回Section名称
    */
    virtual QString sectionName() = 0;

    /*
    获取道路类型
    link表示路段，connector表示连接段
    @return 返回道路类型字符串
    */
    virtual QString sectionType() = 0;

    /*
    获取相关车道或"车道连接"
    @return 返回ILaneObject接口指针列表
    */
    virtual QList<ILaneObject*> laneObjects() = 0;

    /*
    获取相关车道或"车道连接"ID
    @return 返回ID列表
    */
    virtual QList<long> laneObjectIds() = 0;

    /*
    获取上游警示区长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值
    */
    virtual qreal upCautionLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取上游过渡区长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值
    */
    virtual qreal upTransitionLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取上游缓冲区长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值
    */
    virtual qreal upBufferLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取下游过渡区长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值
    */
    virtual qreal downTransitionLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取下游终止区长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值
    */
    virtual qreal downTerminationLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取施工区管控标志序
    @return 返回标志序列表
    */
    virtual QList<int> logoIndexs() = 0;

    /*
    获取初始创建时间
    @return 返回CPU时间（毫秒）
    */
    virtual qint64 createCPUTime() = 0;

    /*
    获取施工持续时间
    自仿真过程创建后，持续时间大于此值，则删除施工区
    @return 返回持续时间（秒）
    */
    virtual long duration() = 0;

    /*
    判断是否被借道
    @return 被借道返回true，否则返回false
    */
    virtual bool isBorrowed() = 0;

    /*
    设置借道状态
    @param b 是否被借道
    */
    virtual void setBorrowed(bool b) = 0;
};

#endif
