/********************************************************************************
* 公交线路接口，由TESS NG实现，用户可以借此访问公交线路对象方法，获取公交线路一些基本信息
*********************************************************************************/

#ifndef __IBusLine__
#define __IBusLine__

#include "tessinterfaces_global.h"
#include "UnitChange.h"
#include <QObject>

class IBusStation;
class ILink;
class IBusStationLine;

class TESSINTERFACES_EXPORT IBusLine
{
public:
	virtual ~IBusLine() = default;

	/*
	获取公交线路ID
	@return 返回公交线路的唯一标识ID
	*/
	virtual long id();

	/*
	获取公交线路名称
	@return 返回公交线路名称
	*/
	virtual QString name();

	/*
	获取公交线路长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回线路长度
	*/
	virtual double length(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取发车间隔
	@return 返回发车间隔（秒）
	*/
	virtual int dispatchFreq();

	/*
	获取发车开始时间
	@return 返回发车开始时间（秒）
	*/
	virtual int dispatchStartTime();

	/*
	获取发车结束时间
	@return 返回发车结束时间（秒）
	*/
	virtual int dispatchEndTime();

	/*
	获取期望速度
	@param unit 单位类型，默认为默认单位（像素/秒）
	@return 返回期望速度
	*/
	virtual qreal desirSpeed(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取起始载客人数
	@return 返回起始载客人数
	*/
	virtual int passCountAtStartTime();

	/*
	获取公交线路经过的路段列表
	@return 返回线路经过的所有ILink接口列表
	*/
	virtual QList<ILink*> links();

	/*
	获取线路所有站点
	@return 返回线路的所有IBusStation接口列表
	*/
	virtual QList<IBusStation*> stations();

	/*
	获取公交站点线路列表
	包含当前站点相关线路的上下客等参数
	@return 返回IBusStationLine接口列表
	*/
	virtual QList<IBusStationLine*> stationLines();

	/*
	设置线路名称
	@param name 线路名称
	*/
	virtual void setName(QString name);

	/*
	设置发车间隔
	@param freq 发车间隔（秒）
	*/
	virtual void setDispatchFreq(int freq);

	/*
	设置发车开始时间
	@param startTime 发车开始时间（秒）
	*/
	virtual void setDispatchStartTime(int startTime);

	/*
	设置发车结束时间
	@param endTime 发车结束时间（秒）
	*/
	virtual void setDispatchEndTime(int endTime);

	/*
	设置线路的期望速度
	@param desirSpeed 期望速度值
	@param unit 单位类型，默认为默认单位（像素/秒）
	*/
	virtual void setDesirSpeed(qreal desirSpeed, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	设置起始载客人数
	@param count 起始载客人数
	*/
	virtual void setPassCountAtStartTime(int count);
};

#endif