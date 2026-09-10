/******************************************************************************
* 模型网络接口，由TESS NG实现，用于管理路网加载和绘制相关功能
*******************************************************************************/

#ifndef __IModelNet__
#define __IModelNet__

#include "tessinterfaces_global.h"

#include <QPainter>

class ILaneObject;
class QGraphicsItem;

class TESSINTERFACES_EXPORT IModelNet
{
public:
    /*
    构造函数
    */
    IModelNet() {}

    /*
    析构函数
    */
    virtual ~IModelNet() {}

    /*
    加载完路网后的处理
    */
    virtual void afterLoadNet();

    /*
    从场景中移除图形项
    @param pItem 要移除的图形项指针
    @return 移除成功返回true，否则返回false
    */
    virtual bool removeItemFromScene(QGraphicsItem* pItem);

    /*
    删除自定义数据
    */
    virtual void deleteCustomerData();

    /*
    绘制车道或车道连接
    @param pILaneObj 车道对象接口指针
    @param painter 绘制器指针
    @return 绘制成功返回true，否则返回false
    */
    virtual bool paintLaneObject(ILaneObject* pILaneObj, QPainter* painter);

};

#endif