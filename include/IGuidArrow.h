/********************************************************************************
* 导向箭头接口，由TESS NG实现，用户可以借此调用接口方法，获取导向箭头的基本属性
*********************************************************************************/

#ifndef __IGuidArrow__
#define __IGuidArrow__

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"
#include "UnitChange.h"
#include <QObject>
#include <QPolygonF>

class ILane;

class TESSINTERFACES_EXPORT IGuidArrow
{
public:
	virtual ~IGuidArrow() = default;

	/*
	获取导向箭头ID
	@return 返回导向箭头的唯一标识ID
	*/
	virtual long id();

	/*
	获取导向箭头所在车道
	@return 返回导向箭头所在的ILane接口
	*/
	virtual ILane* lane();

	/*
	获取导向箭头长度
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回导向箭头长度
	*/
	virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取导向箭头到终点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回到终点的距离
	*/
	virtual qreal distToTerminal(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取导向箭头类型
	@return 返回导向箭头类型（Online::GuideArrowType）
	*/
	virtual Online::GuideArrowType arrowType();

	/*
	获取导向箭头的多边形轮廓
	@return 返回导向箭头的多边形轮廓
	*/
	virtual QPolygonF polygon();
};

#endif

