/******************************************************************************
* 改扩建接口，由TESS NG实现，用于管理道路改扩建相关属性和动态参数
*******************************************************************************/

#ifndef __IReconstruction__
#define __IReconstruction__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"
#include "UnitChange.h"
#include <QObject>

class TESSINTERFACES_EXPORT IReconstruction
{
public:
    virtual ~IReconstruction() = default;

    /*
    获取改扩建ID
    @return 返回改扩建ID
    */
    virtual long id() = 0;

    /*
    获取改扩建起始施工区ID
    @return 返回施工区ID
    */
    virtual long roadWorkZoneId() = 0;

    /*
    获取被借道限行区ID
    @return 返回限行区ID
    */
    virtual long limitedZoneId() = 0;

    /*
    获取保通长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回保通长度（米）
    */
    virtual qreal passagewayLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取改扩建持续时间
    @return 返回持续时间（秒）
    */
    virtual long duration() = 0;

    /*
    获取借道数量
    @return 返回借道数量
    */
    virtual int borrowedNum() = 0;

    /*
    获取保通开口限速
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回限速值（km/h）
    */
    virtual qreal passagewayLimitedSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取初始创建CPU时间
    @return 返回CPU时间（毫秒）
    */
    virtual qint64 createCPUTime() = 0;

    /*
    获取改扩建动态参数
    @return 返回动态参数（米制）
    */
    virtual Online::DynaReconstructionParam dynaReconstructionParam() = 0;
};

#endif
