/********************************************************************************
* 决策点接口，由TESS NG实现，用户可以借此调用接口方法，获取决策点一些基本属性，进行一些操作
*********************************************************************************/

#ifndef __IDecisionPoint__
#define __IDecisionPoint__

#include "tessinterfaces_global.h"
#include "_tables.h"
#include "UnitChange.h"

#include <QPolygonF>

class ILink;
class IRouting;

class TESSINTERFACES_EXPORT IDecisionPoint
{
public:
	virtual ~IDecisionPoint() = default;

	/*
	获取决策点ID
	@return 返回决策点的唯一标识ID
	*/
	virtual long id();

	/*
	获取决策点名称
	@return 返回决策点名称
	*/
	virtual QString name();

	/*
	获取决策点所在路段
	@return 返回决策点所在的ILink接口
	*/
	virtual ILink* link();

	/*
	获取决策点距路段起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回距起点的距离
	*/
	virtual qreal distance(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取相关决策路径列表
	@return 返回包含所有IRouting接口的列表
	*/
	virtual QList<IRouting*> routings();

	/*
	获取决策流量比列表
	@return 返回_RoutingFLowRatio列表
	*/
	virtual QList<_RoutingFLowRatio> routingFLowRatios();

	/*
	获取随机分配比例
	@return 返回随机分配比例（0-1之间的小数）
	*/
	virtual qreal randomRatio();

	/*
	设置是否被动态修改
	默认情况下发车信息被动态修改后，整个文件不能保存，以免破坏原有发车设置
	@param bModified true表示已被动态修改，false表示未被修改
	*/
	virtual void setDynaModified(bool bModified);

	/*
	获取决策点的多边形轮廓
	@return 返回决策点的多边形轮廓
	*/
	virtual QPolygonF polygon();
};

#endif