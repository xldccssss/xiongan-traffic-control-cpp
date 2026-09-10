/******************************************************************************
* 路径接口，由TESS NG实现，用户可以借此调用接口方法，获取路径一些基本属性
*******************************************************************************/

#ifndef __IRouting__
#define __IRouting__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QGraphicsItem>

class ILink;
class ISection;

class TESSINTERFACES_EXPORT IRouting
{
public:
    virtual ~IRouting() = default;

    /*
    获取路径ID
    @return 返回路径ID
    */
    virtual long id();

    /*
    获取所属决策点ID
    @return 返回决策点ID
    */
    virtual long deciPointId();

    /*
    计算路径长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回路径长度值
    */
    virtual qreal calcuLength(UnitOfMeasure unit = UnitOfMeasure::Default);

    /*
    根据所给道路判断是否在当前路径上
    @param pRoad Section接口指针（路段或连接段）
    @return 在当前路径上返回true，否则返回false
    */
    virtual bool contain(ISection* pRoad);

    /*
    根据所给道路求下一条道路
    @param pRoad Section接口指针（路段或连接段）
    @return 返回下一条道路接口指针
    */
    virtual ISection* nextRoad(ISection* pRoad);

    /*
    获取路段序列
    @return 返回路段接口指针列表
    */
    virtual QList<ILink*> getLinks();

    /*
    获取当前仿真时间本路径占比
    @return 返回占比值
    */
    virtual qreal currRatio();

    /*
    设置车辆到达路径终点时是否消失
    默认不消失，自由巡航
    @param bStopped 是否消失
    */
    virtual void setVehiStopAtEnd(bool bStopped);
};

#endif
