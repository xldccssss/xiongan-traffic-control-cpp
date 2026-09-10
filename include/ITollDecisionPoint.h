/******************************************************************************
* 收费决策点接口，由TESS NG实现，用户可以借此调用接口方法，获取收费决策点的基本属性
*******************************************************************************/

#ifndef __ITOLL_DECISION_POINT_H__
#define __ITOLL_DECISION_POINT_H__

#include "tessinterfaces_global.h"
#include "_tables.h"
#include "Plugin/_datastruct.h"

#include <QPolygonF>

class ILink;
class IParkingRouting;
class ITollRouting;
class IRouting;


class TESSINTERFACES_EXPORT ITollDecisionPoint
{
public:
	virtual ~ITollDecisionPoint() = default;

	/*
	获取收费决策点ID
	@return 返回决策点ID
	*/
	virtual long id();

	/*
	获取收费决策点名称
	@return 返回决策点名称
	*/
	virtual QString name();

	/*
	获取收费决策点所在路段
	@return 返回路段接口指针
	*/
	virtual ILink* link();

	/*
	获取距路段起点的距离
	@return 返回距离值（米）
	*/
	virtual qreal distance();

	/*
	获取相关收费路径列表
	@return 返回收费路径接口指针列表
	*/
	virtual QList<ITollRouting*> routings();

	/*
	获取收费决策动态信息列表
	@return 返回收费决策动态信息列表
	*/
	virtual QList<Online::TollStation::DynaTollDisInfo> tollDisInfoList();

	/*
	更新收费决策动态信息列表
	@param 收费决策动态信息列表
	*/
	virtual void updateTollDisInfoList(const QList<Online::TollStation::DynaTollDisInfo>&);

	/*
	获取收费决策点多边形轮廓
	@return 返回多边形轮廓
	*/
	virtual QPolygonF polygon();
};

#endif