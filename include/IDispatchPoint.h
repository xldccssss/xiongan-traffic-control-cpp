/********************************************************************************
* 发车点接口，由TESS NG实现，用户可以借此调用接口方法，获取发车点一些基本属性，增加发车间隔
*********************************************************************************/

#ifndef __IDispatchPoint__
#define __IDispatchPoint__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"

#include <QPolygonF>

class ILink;

class TESSINTERFACES_EXPORT IDispatchPoint {
public:
	virtual ~IDispatchPoint() = default;

	/*
	获取发车点ID
	@return 返回发车点的唯一标识ID
	*/
	virtual long id();

	/*
	获取发车点名称
	@return 返回发车点名称
	*/
	virtual QString name();

	/*
	获取发车点所在路段
	@return 返回发车点所在的ILink接口
	*/
	virtual ILink* link();

	/*
	设置是否被动态修改
	默认情况下发车信息被动态修改后，整个文件不能保存，以免破坏原有发车设置
	@param bModified true表示已被动态修改，false表示未被修改
	*/
	virtual void setDynaModified(bool bModified);

	/*
	增加发车间隔
	@param vehiCompId 车型组成ID
	@param interval 发车间隔（秒）
	@param vehiCount 发车数量
	@return 返回新增发车间隔的ID
	*/
	virtual long addDispatchInterval(long vehiCompId, int interval, int vehiCount);

	/*
	修改发车间隔
	只能修改当前正在仿真的间隔内容及后续间隔内容，当前正在仿真的间隔只能修改发车数，
	且不能导致设定的发车数小于已发车辆数，后续间隔内容可以修改车型组成、发车数
	@param departureIntervalId 发车间隔ID
	@param vehiCount 发车数量
	@param vehiCompId 车型组成ID，默认为0（不修改）
	@return 修改成功返回true，否则返回false
	*/
	virtual bool updateDispatchInterval(long departureIntervalId, int vehiCount, long vehiCompId = 0);

	/*
	删除所有发车信息
	*/
	virtual void removeDispatchIntervals();

	/*
	获取发车间隔列表
	@return 返回Online::DispatchInterval列表
	*/
	virtual QList<Online::DispatchInterval> dispatchIntervals();

	/*
	获取发车点的多边形轮廓
	@return 返回发车点的多边形轮廓
	*/
	virtual QPolygonF polygon();

};

#endif