/******************************************************************************
* 停车决策点接口，由TESS NG实现，用户可以借此调用接口方法，获取停车决策点的基本属性
*******************************************************************************/

#ifndef __IPARKINGDECISIONPOINT_H__
#define __IPARKINGDECISIONPOINT_H__

#include "tessinterfaces_global.h"
#include "_tables.h"
#include "Plugin/_datastruct.h"

#include <QPolygonF>

class ILink;
class IParkingRouting;
class IRouting;


class TESSINTERFACES_EXPORT IParkingDecisionPoint
{
public:
	virtual ~IParkingDecisionPoint() = default;

	/*
	获取停车决策点ID
	@return 返回决策点ID
	*/
	virtual long id();

	/*
	获取停车决策点名称
	@return 返回决策点名称
	*/
	virtual QString name();

	/*
	获取停车决策点所在路段
	@return 返回路段接口指针
	*/
	virtual ILink* link();

	/*
	获取距路段起点的距离
	@return 返回距离值（米）
	*/
	virtual qreal distance();

	/*
	获取相关停车路径列表
	@return 返回停车路径接口指针列表
	*/
	virtual QList<IParkingRouting*> routings();

	/*
	更新停车决策动态信息列表
	@param tollDisInfoList 停车决策动态信息列表
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateParkDisInfo(const QList<Online::ParkingLot::DynaParkDisInfo>& tollDisInfoList);

	/*
	获取停车决策动态信息列表
	@return 返回停车决策动态信息列表
	*/
	virtual QList<Online::ParkingLot::DynaParkDisInfo> parkDisInfoList();

	/*
	获取停车决策点多边形轮廓
	@return 返回多边形轮廓
	*/
	virtual QPolygonF polygon();
};

#endif