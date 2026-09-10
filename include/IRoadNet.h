/******************************************************************************
* 路网接口，由TESS NG实现，用于管理路网的基本信息和元数据
*******************************************************************************/

#ifndef __IRoadNet__
#define __IRoadNet__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QObject>
#include <QJsonObject>

class TESSINTERFACES_EXPORT IRoadNet
{
public:
    virtual ~IRoadNet() = default;

    /*
    获取路网ID
    @return 返回路网ID
    */
    virtual long id();

    /*
    获取路网名称
    @return 返回路网名称
    */
    virtual QString netName();

    /*
    获取源数据路径
    可以是本地文件，也可以是网络地址
    @return 返回源数据路径
    */
    virtual QString url();

    /*
    获取来源分类
    TESSNG表示TESSNG自建、OpenDrive表示由OpenDrive数据导入、geojson表示由geojson数据导入
    @return 返回来源分类字符串
    */
    virtual QString type();

    /*
    获取底图数据路网路径
    可以是本地文件，也可以是网络地址
    @return 返回底图路径
    */
    virtual QString bkgUrl();

    /*
    获取说明
    @return 返回说明文本
    */
    virtual QString explain();

    /*
    获取其它属性JSON数据
    @return 返回JSON对象
    */
    virtual QJsonObject otherAttrs();

    /*
    获取中心点位置
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回中心点坐标
    */
    virtual QPointF centerPoint(UnitOfMeasure unit = UnitOfMeasure::Default);
};

#endif
