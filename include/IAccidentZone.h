/********************************************************************************
* 事故区接口，由TESS NG实现，用户可以借此调用接口方法，获取事故区的基本属性和时段信息
*********************************************************************************/

#ifndef __IAccidentZone__
#define __IAccidentZone__

#include <QObject>
#include <QString>
#include <QList>
#include "tessinterfaces_global.h"
#include "UnitChange.h"
#include "Plugin/_datastruct.h"

class ILaneObject;
class ISection;
class IAccidentZoneInterval;

class TESSINTERFACES_EXPORT IAccidentZone
{
public:
	virtual ~IAccidentZone() = default;

	/*
	获取事故区ID
	@return 返回事故区的唯一标识ID
	*/
	virtual long id() = 0;

	/*
	获取事故区名称
	@return 返回事故区名称
	*/
	virtual QString name() = 0;

	/*
	获取事故区当前时段距起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距起点的距离
	*/
	virtual qreal location(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取事故区当前时段的长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回事故区长度
	*/
	virtual qreal zoneLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取事故区所在的路段或连接段
	@return 返回ISection接口，可能是路段或连接段
	*/
	virtual ISection* section() = 0;

	/*
	获取道路ID
	@return 返回道路的唯一标识ID
	*/
	virtual long roadId() = 0;

	/*
	获取道路类型
	@return 返回道路类型字符串，"路段"或"连接段"
	*/
	virtual QString roadType() = 0;

	/*
	获取事故区当前时段占用的车道对象列表
	@return 返回占用车道的ILaneObject列表
	*/
	virtual QList<ILaneObject*> laneObjects() = 0;

	/*
	获取事故区当前时段的限速
	@param unit 单位类型，默认为默认单位（km/h）
	@return 返回限速值
	*/
	virtual qreal limitedSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取事故区当前时段的控制距离
	车辆距离事故区起点该距离内，强制变道
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回控制距离
	*/
	virtual qreal controlLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	添加事故时段
	@param param 事故时段参数
	@return 返回新增的事故时段接口
	*/
	virtual IAccidentZoneInterval* addAccidentZoneInterval(Online::DynaAccidentZoneIntervalParam param) = 0;

	/*
	移除事故时段
	@param accidentZoneIntervalId 要移除的事故时段ID
	*/
	virtual void removeAccidentZoneInterval(long accidentZoneIntervalId) = 0;

	/*
	更新事故时段
	@param param 更新后的事故时段参数
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateAccidentZoneInterval(Online::DynaAccidentZoneIntervalParam param) = 0;

	/*
	获取所有事故时段
	@return 返回所有事故时段的接口列表
	*/
	virtual QList<IAccidentZoneInterval*> accidentZoneIntervals() = 0;

	/*
	根据ID查询事故时段
	@param accidentZoneIntervalId 事故时段ID
	@return 返回匹配的事故时段接口，若无则返回空指针
	*/
	virtual IAccidentZoneInterval* findAccidentZoneIntervalById(long accidentZoneIntervalId) = 0;

	/*
	根据起始时间查询事故时段
	@param startTime 时段起始时间戳
	@return 返回匹配的事故时段接口，若无则返回空指针
	*/
	virtual IAccidentZoneInterval* findAccidentZoneIntervalByStartTime(long startTime) = 0;
};

#endif

