/******************************************************************************
* 停车决策路径接口，由TESS NG实现，用户可以借此调用接口方法，获取停车路径的基本属性
*******************************************************************************/

#ifndef __IPARKINGROUTING_H__
#define __IPARKINGROUTING_H__

#include "tessinterfaces_global.h"
#include "_tables.h"

#include <QPolygonF>

class ILink;
class ISection;

class TESSINTERFACES_EXPORT IParkingRouting
{
public:
	virtual ~IParkingRouting() = default;

	/*
	获取停车路径ID
	@return 返回路径ID
	*/
	virtual long id();

	/*
	获取所属停车决策点ID
	@return 返回停车决策点ID
	*/
	virtual long parkingDeciPointId();

	/*
	获取路径到达的停车区域ID
	@return 返回停车区域ID
	*/
	virtual long parkingRegionId();

	/*
	计算路径长度
	@return 返回路径长度
	*/
	virtual qreal calcuLength();

	/*
	根据所给道路判断是否在当前停车路径上
	@param pRoad 道路接口指针（路段或连接段）
	@return 在路径上返回true，否则返回false
	*/
	virtual bool contain(ISection* pRoad);

	/*
	根据当前道路获取下一条道路
	@param pRoad 当前道路接口指针（路段或连接段）
	@return 返回下一条道路接口指针
	*/
	virtual ISection* nextRoad(ISection* pRoad);

	/*
	获取路段序列
	@return 返回路段接口指针列表
	*/
	virtual QList<ILink*> getLinks();
};

#endif