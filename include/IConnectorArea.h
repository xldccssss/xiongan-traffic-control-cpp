/********************************************************************************
* 面域接口，由TESS NG实现，用户可以借此调用接口方法，获取面域一些基本属性
*********************************************************************************/

#ifndef __IConnectorArea__
#define __IConnectorArea__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QPointF>
#include <QList>

class IConnector;

class TESSINTERFACES_EXPORT IConnectorArea
{
public:
	virtual ~IConnectorArea() = default;

	/*
	获取面域ID
	@return 返回面域的唯一标识ID
	*/
	virtual long id() = 0;

	/*
	获取面域中心点坐标
	@param unit 单位类型，默认为默认单位
	@return 返回中心点坐标（QPointF）
	*/
	virtual QPointF centerPoint(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取面域内所有连接段
	@return 返回包含所有IConnector接口的列表
	*/
	virtual QList<IConnector*> allConnector() = 0;

	/*
	获取 Worker 标识符
	@return 返回Worker标识符
	*/
	virtual int workerKey() = 0;

	/*
	设置 Worker 标识符
	@param key Worker标识符
	*/
	virtual void setWorkerKey(int key) = 0;
};

#endif