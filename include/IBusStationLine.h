/********************************************************************************
* 公交站点线路接口，由TESS NG实现，用户可以借此访问公交站点线路对象方法，获取站点线路的上下客等参数
*********************************************************************************/

#ifndef __IBusStationLine__
#define __IBusStationLine__

#include "tessinterfaces_global.h"

#include <QObject>

class TESSINTERFACES_EXPORT IBusStationLine
{
public:
	virtual ~IBusStationLine() = default;

	/*
	获取站点线路ID
	@return 返回站点线路的唯一标识ID
	*/
	virtual long id();

	/*
	获取公交站点ID
	@return 返回所属公交站点的ID
	*/
	virtual long stationId();

	/*
	获取公交线路ID
	@return 返回所属公交线路的ID
	*/
	virtual long lineId();

	/*
	获取车辆停靠时间
	@return 返回车辆停靠时间（秒）
	*/
	virtual int busParkingTime();

	/*
	获取下客百分比
	@return 返回下客百分比（0-1之间的小数）
	*/
	virtual qreal getOutPercent();

	/*
	获取平均每位乘客上车时间
	@return 返回平均每位乘客上车时间
	*/
	virtual qreal getOnTimePerPerson();

	/*
	获取平均每位乘客下车时间
	@return 返回平均每位乘客下车时间
	*/
	virtual qreal getOutTimePerPerson();

	/*
	设置车辆停靠时间
	@param time 停靠时间（秒）
	*/
	virtual void setBusParkingTime(int time);

	/*
	设置下客百分比
	@param percent 下客百分比（0-1之间的小数）
	*/
	virtual void setGetOutPercent(qreal percent);

	/*
	设置平均每位乘客上车时间
	@param time 平均上车时间
	*/
	virtual void setGetOnTimePerPerson(qreal time);

	/*
	设置平均每位乘客下车时间
	@param time 平均下车时间
	*/
	virtual void setGetOutTimePerPerson(qreal time);

};

#endif