/******************************************************************************
* 模型GUI接口，由TESS NG实现，用于管理模型的界面交互功能
*******************************************************************************/

#ifndef __IModelGui__
#define __IModelGui__

#include "tessinterfaces_global.h"

class QAction;

class TESSINTERFACES_EXPORT IModelGui
{
public:
    /*
    构造函数
    */
    IModelGui() {}

    /*
    初始化GUI
    @param isCerted 是否已认证
    */
    virtual void init(bool isCerted) {}

    /*
    析构函数
    */
    virtual ~IModelGui() {}

    /*
    获取QAction列表
    @return 返回QAction指针列表
    */
    virtual QList<QAction*> actions() { return {}; };

    /*
    激活软件后的处理
    @param isCerted 是否已认证，默认为false
    */
    virtual void afterActiveSoftware(bool isCerted = false) { return; };
};

#endif