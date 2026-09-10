/*****************************************************************************
* 客户自定义路网元素基础类
* 定义一些抽象方法，用户借此创建可视化元素并实现这些方法后，可以在插件环境下与TESS NG进行交互
******************************************************************************/

#ifndef __CustomerGraphicsObject__
#define __CustomerGraphicsObject__

#include "tessinterfaces_global.h"

#include <QGraphicsObject>
#include <QSqlQuery>
#include <QSqlError>

class GraphicsScene;
class RoadNetScene;
class ISection;

class TESSINTERFACES_EXPORT CustomerGraphicsObject : public QGraphicsObject
{
    Q_OBJECT

public:
    /*
    构造函数
    @param parent 父图形项指针，默认为nullptr
    */
    CustomerGraphicsObject(QGraphicsItem* parent = nullptr);

    /*
    析构函数
    */
    virtual ~CustomerGraphicsObject();

    /*
    复制当前对象
    @param linkId 所属路段ID
    @param addToScene 是否添加到场景
    @return 返回复制后的对象指针
    */
    virtual CustomerGraphicsObject* copyThis(qlonglong linkId, bool addToScene) { return nullptr; };

    /*
    获取标识符
    @return 返回标识符ID
    */
    virtual long identification() { return -1; };

    /*
    获取图形项类型
    @return 返回类型枚举值
    */
    virtual int type() const override { return QGraphicsItem::UserType + 1000; }

    /*
    获取表名
    @return 返回表名字符串
    */
    virtual QString tableName() { return QString(); }

    /*
    获取距起点距离
    @return 返回距离值
    */
    virtual qreal distToStartPoint() { return 0; }

    /*
    获取长度
    @return 返回长度值
    */
    virtual qreal length() { return 0; }

    /*
    插入记录到数据库
    @param sqlDb SQL数据库对象
    @return 插入成功返回true，否则返回false
    */
    virtual bool insertRecord(QSqlDatabase& sqlDb) { return true; };

    /*
    获取所属道路
    @return 返回所属道路接口指针
    */
    virtual ISection* road() { return nullptr; };

    /*
    调整对象
    */
    virtual void adjust() {}

    /*
    检测对象状态
    */
    virtual void detect() {}

    /*
    计算字体大小
    @param fsize 原始字体大小
    @return 返回计算后的字体大小
    */
    qreal calcFontSize(qreal fsize) const;

    /*
    清除子项
    */
    virtual void clearSubItem() {}

    /*
    恢复数据
    @param oldData 旧数据指针
    @param oper 操作类型
    */
    virtual void restoreData(void* oldData, int oper) {}

    /*
    扩展功能
    @param variant 输入参数
    @return 返回扩展结果
    */
    virtual QVariant extension(const QVariant& variant) const { return {}; };

public:
    /*
    获取边界矩形
    @return 返回边界矩形
    */
    QRectF boundingRect() const;

    /*
    获取形状路径
    @return 返回形状路径
    */
    QPainterPath shape() const;

    /*
    绘制对象
    @param painter 绘制器指针
    @param option 样式选项指针
    @param widget 窗口部件指针
    */
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

};

#endif