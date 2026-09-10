/******************************************************************************
* 收费点接口，由TESS NG实现，用户可以借此调用接口方法，获取收费点的基本属性
*******************************************************************************/

#ifndef __TOLL_POINT_H__
#define __TOLL_POINT_H__

#include "tessinterfaces_global.h"
#include <QGraphicsItem>

class ILink;
class ISection;
class ITollLane;

class TESSINTERFACES_EXPORT ITollPoint
{
public:
	virtual ~ITollPoint() = default;

	/*
	获取收费点ID
	@return 返回收费点ID
	*/
	virtual long id() = 0;

	/*
	获取距路段起始位置的距离
	@return 返回距离值（米）
	*/
	virtual qreal distance() const = 0;

	/*
	获取所属收费车道ID
	@return 返回收费车道ID
	*/
	virtual long tollLaneId() = 0;

	/*
	获取收费车道接口指针
	@return 返回收费车道接口指针
	*/
	virtual ITollLane* tollLane() = 0;

	/*
	判断收费点是否启用
	@return 启用返回true，否则返回false
	*/
	virtual bool isEnabled() = 0;

	/*
	设置收费点启用状态
	@param enabled 是否启用
	@return 设置成功返回true，否则返回false
	*/
	virtual bool setEnabled(bool enabled) = 0;

	/*
	获取收费类型
	@return 返回收费类型
	*/
	virtual int tollType() = 0;

	/*
	设置收费类型
	@param tollType 收费类型
	@return 设置成功返回true，否则返回false
	*/
	virtual bool setTollType(int tollType) = 0;

	/*
	获取停车时间分布ID
	@return 返回停车时间分布ID
	*/
	virtual int timeDisId() = 0;

	/*
	设置停车时间分布ID
	@param timeDisId 停车时间分布ID
	@return 设置成功返回true，否则返回false
	*/
	virtual bool setTimeDisId(int timeDisId) = 0;
};

#endif