/******************************************************************************
* 限速区接口，由TESS NG实现，用于管理限速区属性及限速时段配置
*******************************************************************************/

#ifndef __IReduceSpeedArea__
#define __IReduceSpeedArea__

#include "tessinterfaces_global.h"
#include "UnitChange.h"
#include "Plugin/_datastruct.h"

#include <QObject>

class IReduceSpeedInterval;

class TESSINTERFACES_EXPORT IReduceSpeedArea
{
public:
    virtual ~IReduceSpeedArea() = default;

    /*
    获取限速区ID
    @return 返回限速区ID
    */
    virtual long id() = 0;

    /*
    获取限速区名称
    @return 返回限速区名称
    */
    virtual QString name() = 0;

    /*
    获取距起点距离
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回距离值
    */
    virtual qreal location(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取限速区长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值
    */
    virtual qreal areaLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取路段或连接段ID
    @return 返回Section ID
    */
    virtual long sectionId() = 0;

    /*
    获取车道序号
    @return 返回车道序号
    */
    virtual int laneNumber() = 0;

    /*
    获取目标车道序号
    @return 返回目标车道序号
    */
    virtual int toLaneNumber() = 0;

    /*
    添加限速时段
    @param param 限速时段参数
    @return 返回新建的限速时段接口指针
    */
    virtual IReduceSpeedInterval* addReduceSpeedInterval(Online::DynaReduceSpeedIntervalParam param) = 0;

    /*
    移除限速时段
    @param id 限速时段ID
    */
    virtual void removeReduceSpeedInterval(long id) = 0;

    /*
    更新限速时段
    @param param 限速时段参数
    @return 更新成功返回true，否则返回false
    */
    virtual bool updateReduceSpeedInterval(Online::DynaReduceSpeedIntervalParam param) = 0;

    /*
    获取限速时段列表
    @return 返回限速时段接口指针列表
    */
    virtual QList<IReduceSpeedInterval*> reduceSpeedIntervals() = 0;

    /*
    根据ID获取限速时段
    @param id 限速时段ID
    @return 返回限速时段接口指针
    */
    virtual IReduceSpeedInterval* findReduceSpeedIntervalById(long id) = 0;

    /*
    根据起始时间获取限速时段
    @param startTime 起始时间（秒）
    @return 返回限速时段接口指针
    */
    virtual IReduceSpeedInterval* findReduceSpeedIntervalByStartTime(long startTime) = 0;

    /*
    获取多边形轮廓
    @return 返回多边形
    */
    virtual QPolygonF polygon() = 0;
};

#endif
