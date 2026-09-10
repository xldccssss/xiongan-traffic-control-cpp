/********************************************************************************
* 节点接口，由TESS NG实现，用户可以借此调用接口方法，获取节点的基本属性和流向信息
*********************************************************************************/

#ifndef __IJUNCTION_H__
#define __IJUNCTION_H__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"

#include <QGraphicsItem>
#include <QString>
#include <QList>

class ILink;
class IConnector;

class TESSINTERFACES_EXPORT IJunction
{
public:
	virtual ~IJunction() = default;

	/*
	获取节点ID
	@return 返回节点的唯一标识ID
	*/
	virtual long getId() const = 0;

	/*
	获取节点名称
	@return 返回节点名称
	*/
	virtual QString name() const = 0;

	/*
	设置节点名称
	@param strName 新的节点名称
	*/
	virtual void setName(const QString& strName) const = 0;

	/*
	获取节点内的路段列表
	@return 返回节点内所有ILink接口的列表
	*/
	virtual QList<ILink*> getJunctionLinks() const = 0;

	/*
	获取节点内的连接段列表
	@return 返回节点内所有IConnector接口的列表
	*/
	virtual QList<IConnector*> getJunctionConnectors() const = 0;

	/*
	获取节点内的所有流向信息
	@return 返回所有流向信息的列表
	*/
	virtual QList<Online::Junction::TurnningBaseInfo> getAllTurningInfo() const = 0;

	/*
	获取节点内指定的流向信息
	@param turningId 流向ID
	@return 返回指定流向的信息
	*/
	virtual Online::Junction::TurnningBaseInfo getTurningInfo(long turningId) const = 0;

};

#endif