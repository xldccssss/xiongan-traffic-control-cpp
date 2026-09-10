/******************************************************************************
* 收费车道接口，由TESS NG实现，用户可以借此调用接口方法，获取收费车道的基本属性
*******************************************************************************/

#ifndef __TOLL_LANE_H_
#define __TOLL_LANE_H_

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"

#include <QGraphicsItem>
#include <QObject>

class ILink;
class ISection;
class ITollPoint;

class TESSINTERFACES_EXPORT ITollLane
{
public:
	virtual ~ITollLane() = default;

	/*
	获取收费车道ID
	@return 返回收费车道ID
	*/
	virtual long id() const;

	/*
	获取收费车道名称
	@return 返回收费车道名称
	*/
	virtual QString name() const;

	/*
	获取距路段起始位置的距离
	@return 返回距离值（米）
	*/
	virtual qreal distance() const;

	/*
	设置收费车道名称
	@param name 收费车道新名称
	*/
	virtual void setName(const QString& name);
	
	/*
	设置工作时间，工作时间与仿真时间对应
	@param startTime 开始时间（秒）
	@param endTime 结束时间（秒）
	*/
	virtual void setWorkTime(long startTime, long endTime);

	/*
	获取收费车道动态信息
	@return 返回收费车道动态信息结构体
	*/
	virtual Online::TollStation::DynaTollLane dynaTollLane();

	/*
	获取收费车道所有收费点
	@return 返回收费点接口指针列表
	*/
	virtual QList<ITollPoint*> tollPoints() const;
};

#endif
