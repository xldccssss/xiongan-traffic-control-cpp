/********************************************************************************
* 限行区接口，由TESS NG实现，用户可以借此调用接口方法，获取限行区的基本属性
* 包括施工区、限速区等各类限行区域
*********************************************************************************/

#ifndef __ILimitedZone__
#define __ILimitedZone__

#include <QObject>
#include <QString>
#include <QList>

#include "tessinterfaces_global.h"
#include "UnitChange.h"

class ILaneObject;
class ISection;

class TESSINTERFACES_EXPORT ILimitedZone
{
public:
	virtual ~ILimitedZone() = default;

	/*
	获取限行区ID
	@return 返回限行区的唯一标识ID
	*/
	virtual long id() = 0;

	/*
	获取限行区名称
	@return 返回限行区名称
	*/
	virtual QString name() = 0;

	/*
	获取限行区位置（距起点距离）
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回限行区起点距所属路段起点的距离
	*/
	virtual qreal location(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取限行区长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回限行区的长度
	*/
	virtual qreal zoneLength(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取限行区限速（最大车速）
	@param unit 单位类型，默认为默认单位（像素/秒）
	@return 返回限行区内的最大允许车速
	*/
	virtual qreal limitSpeed(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

	/*
	获取所属路段或连接段ID
	@return 返回所属Section的ID
	*/
	virtual long sectionId() = 0;

	/*
	获取所属Section名称
	@return 返回所属Section的名称
	*/
	virtual QString sectionName() = 0;

	/*
	获取道路类型
	@return 返回道路类型字符串，"link"表示路段，"connector"表示连接段
	*/
	virtual QString sectionType() = 0;

	/*
	获取限行区内的车道对象列表
	@return 返回ILaneObject列表
	*/
	virtual QList<ILaneObject*> laneObjects() = 0;

	/*
	获取限行持续时间
	自仿真过程创建后，持续时间超过此值则自动删除限行区
	@return 返回限行持续时间（秒）
	*/
	virtual long duration() = 0;
};

#endif