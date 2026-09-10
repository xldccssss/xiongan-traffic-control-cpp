/******************************************************************************
* 停车位接口，由TESS NG实现，用户可以借此调用接口方法，获取停车位的基本属性
*******************************************************************************/

#ifndef __PARKING_STALL__
#define __PARKING_STALL__

#include "tessinterfaces_global.h"

#include <QGraphicsItem>

class ILink;
class ISection;
class IParkingRegion;

class TESSINTERFACES_EXPORT IParkingStall
{
public:
	virtual ~IParkingStall() = default;

	/*
	获取停车位ID
	@return 返回停车位ID
	*/
	virtual long id();

	/*
	获取所属停车区域ID
	@return 返回停车区域ID
	*/
	virtual long parkingRegionId();

	/*
	获取停车区域接口指针
	@return 返回停车区域接口指针
	*/
	virtual IParkingRegion* parkingRegion() = 0;

	/*
	获取距路段起始位置的距离
	@return 返回距离值（米）
	*/
	virtual qreal distance() const = 0;

	/*
	获取车位类型
	与车辆类型编码一致
	@return 返回车位类型
	*/
	virtual int stallType() = 0;

	/*
	判断车位是否预分配使用
	@return 已预分配返回true，否则返回false
	*/
	virtual bool isPreUsed() = 0;

	/*
	判断车位是否使用
	@return 已使用返回true，否则返回false
	*/
	virtual bool isUsed() = 0;
};

#endif