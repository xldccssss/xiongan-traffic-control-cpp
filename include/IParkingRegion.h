/******************************************************************************
* 停车区域接口，由TESS NG实现，用户可以借此调用接口方法，获取停车区域的基本属性
*******************************************************************************/

#ifndef __PARKINGREGION_STALL__
#define __PARKINGREGION_STALL__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"

#include <QGraphicsItem>

class ILink;
class IParkingStall;

class TESSINTERFACES_EXPORT IParkingRegion
{
public:
	virtual ~IParkingRegion() = default;

	/*
	获取停车区域ID
	@return 返回停车区域ID
	*/
	virtual long id() const;

	/*
	获取停车区域名称
	@return 返回停车区域名称
	*/
	virtual QString name() const;

	/*
	设置停车区域名称
	@param name 新名称
	*/
	virtual void setName(const QString& name);

	/*
	获取所有停车位
	@return 返回停车位接口指针列表
	*/
	virtual QList<IParkingStall*> parkingStalls() const;

	/*
	获取停车区域动态信息
	@return 返回停车区域动态信息结构体
	*/
	virtual Online::ParkingLot::DynaParkingRegion dynaParkingRegion();

	/*
	获取停车区域多边形轮廓
	@return 返回多边形轮廓
	*/
	virtual QPolygonF polygon();
};

#endif