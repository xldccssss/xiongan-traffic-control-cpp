/***********************************************************************
 * 车辆监视接口，TESS NG实现，用户借此接口可以获取车辆与路段、连接段、车道、"车道连接"之间关系等
 ***********************************************************************/

#ifndef IVEHICLESUPERVISOR_H
#define IVEHICLESUPERVISOR_H

#include "tessinterfaces_global.h"

#include <QObject>

class IVehicle;

class TESSINTERFACES_EXPORT IVehicleSupervisor
{
public:
	virtual ~IVehicleSupervisor() = default;

	/*
	获取指定路段上的所有车辆
	@param linkId 路段ID
	@return 返回路段上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInLink(long linkId);

	/*
	获取指定车道上的所有车辆
	@param laneId 车道ID
	@return 返回车道上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInLane(long laneId);

	/*
	获取指定连接段上的所有车辆
	@param connectorId 连接段ID
	@return 返回连接段上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInConnector(long connectorId);

	/*
	获取指定车道连接器上的所有车辆
	@param connectorId 连接段ID
	@param fromLaneId 上游车道ID
	@param toLaneId 下游车道ID
	@return 返回车道连接器上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInLaneConnector(long connectorId, long fromLaneId, long toLaneId);

	/*
	计算两车辆多边形之间的最近距离
	@param pV1 第一辆车接口指针
	@param pV2 第二辆车接口指针
	@return 返回两车辆之间的最近距离（单位：像素）
	*/
	virtual qreal distanceTwoVehicle(IVehicle *pV1, IVehicle *pV2);

private:
	
};

#endif // IVEHICLESUPERVISOR_H