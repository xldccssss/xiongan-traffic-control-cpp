/********************************************************************************
* 评价区域接口，由TESS NG实现，用户可以借此调用接口方法，获取评价区域的基本属性
*********************************************************************************/

#ifndef __IEvaluateArea_H__
#define __IEvaluateArea_H__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"

#include <QGraphicsItem>
#include <QString>

class ILink;
class IConnector;

class TESSINTERFACES_EXPORT IEvaluateArea
{
public:
	virtual ~IEvaluateArea() = default;

	/*
	获取评价区域ID
	@return 返回评价区域的唯一标识ID
	*/
	virtual long getId() = 0;

	/*
	获取评价区域名称
	@return 返回评价区域名称
	*/
	virtual QString name() = 0;

	/*
	设置评价区域名称
	@param strName 新的评价区域名称
	*/
	virtual void setName(const QString& strName) = 0;
};

#endif