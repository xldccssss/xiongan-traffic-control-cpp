/******************************************************************************
* Section接口，由TESS NG实现，用于管理路段(Link)和连接段(Connector)的统一访问
* Section可以是Link也可以是Connector，通过接口方法可以区分并获取相关信息
*******************************************************************************/

#ifndef __ISection__
#define __ISection__

#include "tessinterfaces_global.h"
#include "UnitChange.h"

#include <QPolygonF>

class ILaneObject;
class ILink;
class IConnector;

class TESSINTERFACES_EXPORT ISection
{
public:
    virtual ~ISection() = default;

    /*
    获取Section ID
    如果是Link，ID是Link的ID；如果是连接段，ID是连接段ID+10000000，从而区分路段与连接段
    @return 返回Section ID
    */
    virtual long sectionId() = 0;

    /*
    获取原始ID
    如果是Link，ID是Link的ID；如果是连接段，ID是连接段ID
    @return 返回原始ID
    */
    virtual long id() = 0;

    /*
    获取类型
    取值为GLinkType或GConnectorType
    @return 返回类型枚举值
    */
    virtual int gtype() = 0;

    /*
    判断是否为路段(Link)
    @return 是路段返回true，否则返回false
    */
    virtual bool isLink() = 0;

    /*
    获取Section名称
    @return 返回Section名称
    */
    virtual QString name() = 0;

    /*
    设置Section名称
    @param name Section名称
    */
    virtual void setName(QString name) = 0;

    /*
    获取高程
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回高程值
    */
    virtual qreal v3z(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取长度
    @param unit 单位类型，默认为默认单位（像素）
    @return 返回长度值（米）
    */
    virtual qreal length(UnitOfMeasure unit = UnitOfMeasure::Default) = 0;

    /*
    获取ILaneObject列表
    @return 返回ILaneObject接口指针列表
    */
    virtual QList<ILaneObject*> laneObjects() = 0;

    /*
    根据ID获取上游Section
    如果当前是路段，id为0返回空指针，否则返回上游指定ID的连接段；
    如果当前是连接段，id为0返回上游路段，否则返回空指针。
    @param id 上游Section ID，默认为0
    @return 返回上游Section接口指针
    */
    virtual ISection* fromSection(long id = 0) = 0;

    /*
    根据ID获取下游Section
    如果当前是路段，id为0返回空指针，否则返回下游指定ID的连接段；
    如果当前是连接段，id为0返回下游路段，否则返回空指针。
    @param id 下游Section ID，默认为0
    @return 返回下游Section接口指针
    */
    virtual ISection* toSection(long id = 0) = 0;

    /*
    设置路段或连接段其它属性
    @param otherAttr JSON对象格式的属性数据
    */
    virtual void setOtherAttr(QJsonObject otherAttr) = 0;

    /*
    转换成ILink
    如果当前为连接段则返回空指针
    @return 返回ILink接口指针
    */
    virtual ILink* castToLink() = 0;

    /*
    转换成IConnector
    如果当前为路段则返回空指针
    @return 返回IConnector接口指针
    */
    virtual IConnector* castToConnector() = 0;

    /*
    获取多边形轮廓
    @return 返回多边形
    */
    virtual QPolygonF polygon() = 0;

    /*
    获取Worker节点标识
    @return 返回Worker节点标识
    */
    virtual int workerKey() = 0;

    /*
    设置Worker节点
    @param key Worker节点标识
    */
    virtual void setWorkerKey(int key) = 0;

    /*
    获取上游Worker节点标识
    @return 返回上游Worker节点标识
    */
    virtual int fromWorkerKey() = 0;

    /*
    设置上游Worker节点标识
    @param key 上游Worker节点标识
    */
    virtual void setFromWorkerKey(int key) = 0;
};

#endif
