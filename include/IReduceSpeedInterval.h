/******************************************************************************
* 限速时段接口，由TESS NG实现，用于管理限速区中的限速时段及其限速车型
*******************************************************************************/

#ifndef __IReduceSpeedInterval_H__
#define __IReduceSpeedInterval_H__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"

#include <QObject>

class IReduceSpeedVehiType;

class TESSINTERFACES_EXPORT IReduceSpeedInterval
{
public:
    virtual ~IReduceSpeedInterval() {}

    /*
    获取限速时段ID
    @return 返回限速时段ID
    */
    virtual long id() = 0;

    /*
    获取所属限速区ID
    @return 返回限速区ID
    */
    virtual long reduceSpeedAreaId() = 0;

    /*
    获取开始时间
    @return 返回开始时间（秒）
    */
    virtual long intervalStartTime() = 0;

    /*
    获取结束时间
    @return 返回结束时间（秒）
    */
    virtual long intervalEndTime() = 0;

    /*
    添加限速车型
    @param param 限速车型参数
    @return 返回新建的限速车型接口指针
    */
    virtual IReduceSpeedVehiType* addReduceSpeedVehiType(Online::DynaReduceSpeedVehiTypeParam param) = 0;

    /*
    移除限速车型
    @param id 限速车型ID
    */
    virtual void removeReduceSpeedVehiType(long id) = 0;

    /*
    更新限速车型
    @param param 限速车型参数
    @return 更新成功返回true，否则返回false
    */
    virtual bool updateReduceSpeedVehiType(Online::DynaReduceSpeedVehiTypeParam param) = 0;

    /*
    获取本时段限速车型及限速参数列表
    @return 返回限速车型接口指针列表
    */
    virtual QList<IReduceSpeedVehiType*> reduceSpeedVehiTypes() = 0;

    /*
    根据ID获取限速车型
    @param id 限速车型ID
    @return 返回限速车型接口指针
    */
    virtual IReduceSpeedVehiType* findReduceSpeedVehiTypeById(long id) = 0;

    /*
    根据车型代码获取限速车型
    @param vehicleTypeCode 车型代码
    @return 返回限速车型接口指针
    */
    virtual IReduceSpeedVehiType* findReduceSpeedVehiTypeByCode(long vehicleTypeCode) = 0;
};

#endif
