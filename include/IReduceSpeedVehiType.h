/******************************************************************************
* 限速车型接口，由TESS NG实现，用于管理限速时段中特定车型的限速参数
*******************************************************************************/

#ifndef __IReduceSpeedVehiType_H__
#define __IReduceSpeedVehiType_H__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QObject>

class TESSINTERFACES_EXPORT IReduceSpeedVehiType
{
public:
    virtual ~IReduceSpeedVehiType() {}

    /*
    获取限速车型ID
    @return 返回限速车型ID
    */
    virtual long id() = 0;

    /*
    获取所属限速时段ID
    @return 返回限速时段ID
    */
    virtual long intervalId() = 0;

    /*
    获取所属限速区ID
    @return 返回限速区ID
    */
    virtual long reduceSpeedAreaId() = 0;

    /*
    获取车型编码
    @return 返回车型编码
    */
    virtual long vehiTypeCode() = 0;

    /*
    获取平均车速
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回平均车速（像素/秒）
    */
    virtual qreal averageSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取车速标准差
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回车速标准差（像素/秒）
    */
    virtual qreal speedStandardDeviation(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;
};

#endif
