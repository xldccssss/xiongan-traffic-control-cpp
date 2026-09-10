/********************************************************************************
* 用户插件参与在路网层与TESS NG交互，用户实现全部或部分方法扩展TESS NG的运用
*********************************************************************************/

#ifndef __CustomerNet__
#define __CustomerNet__

#include "../tessinterfaces_global.h"
#include "_netitem.h"
#include "tessplugin.h"

#include <QObject>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>

class ILink;
class CustomerGraphicsObject;
class ILaneObject;

class TESSINTERFACES_EXPORT CustomerNet
{
public:
    /*
    构造函数
    */
    CustomerNet() {}

    /*
    析构函数
    */
    virtual ~CustomerNet();

    /*
    获取路网文件后缀
    @param suffix 输出文件后缀字符串
    @return 成功返回true，否则返回false
    */
    virtual bool netFileSuffix(QString &suffix);

    /*
    获取用户设计的表
    @return 返回表名到DDL语句的映射
    */
    virtual QMap<QString, QString> customerTableDDL();

    /*
    插入用户插件创建的表数据
    */
    virtual void insertCustomerData();

    /*
    删除用户插件创建的表数据
    */
    virtual void deleteCustomerData();

    /*
    加载路网前的准备
    */
    virtual void beforeLoadNet();

    /*
    加载完路网后的处理
    */
    virtual void afterLoadNet();

    /*
    从场景中移除插件创建的对象
    @param pItem 要移除的图形对象指针
    @return 移除成功返回true，否则返回false
    */
    virtual bool removeItemFromScene(CustomerGraphicsObject *pItem);

    /*
    获取路段类型
    @param lType 输出路段类型列表
    @return 成功返回true，否则返回false
    */
    virtual bool linkType(QList<QString> &lType);

    /*
    获取车道类型
    @param lType 输出车道类型列表
    @return 成功返回true，否则返回false
    */
    virtual bool laneType(QList<QString> &lType);

    /*
    创建车道
    @param pILink 路段接口指针
    @return 创建成功返回true，否则返回false
    */
    virtual bool linkBuildGLanes(ILink *pILink);

    /*
    获取曲化精度
    @param itemType 项目类型（GLinkType或GConnectorType）
    @param itemId 路段或连接段ID
    @param extend1 输出延伸距离1
    @param extend2 输出延伸距离2
    @param precision 输出精度值
    @return 成功返回true，否则返回false
    */
    virtual bool curvaturePrecision(int itemType, long itemId, qreal &extend1, qreal &extend2, qreal &precision);

    /*
    获取曲化最短距离
    @param itemType 项目类型（GLinkType或GConnectorType）
    @param itemId 路段或连接段ID
    @param minDist 输出最短距离（米）
    @return 成功返回true，否则返回false
    */
    virtual bool curvatureMinDist(int itemType, long itemId, qreal& minDist);

    /*
    判断是否允许调用客户绘制
    默认为true。本方法的目的是：在python环境减少不必要的对python方法调用，消除对运行效率的负面影响
    @return 允许返回true，否则返回false
    */
    virtual bool isPermitForCustDraw();

    /*
    绘制路网元素
    @param itemType 项目类型
    @param itemId 项目ID
    @param painter 绘制器指针
    @return 绘制成功返回true，否则返回false
    */
    virtual bool paint(int itemType, long itemId, QPainter *painter);

    /*
    绘制车道或车道连接
    @param pILaneObj 车道对象接口指针
    @param painter 绘制器指针
    @return 绘制成功返回true，否则返回false
    */
    virtual bool paintLaneObject(ILaneObject* pILaneObj, QPainter* painter);

    /*
    获取路段brush颜色
    @param linkId 路段ID
    @param color 输出颜色值
    @return 成功返回true，否则返回false
    */
    virtual bool linkBrushColor(long linkId, QColor& color);

    /*
    获取车道brush颜色
    @param laneId 车道ID
    @param color 输出颜色值
    @return 成功返回true，否则返回false
    */
    virtual bool laneBrushColor(long laneId, QColor& color);

    /*
    获取车道brush和pen
    @param laneId 车道ID
    @param brush 输出画刷对象
    @param pen 输出画笔对象
    @return 成功返回true，否则返回false
    */
    virtual bool laneBrushAndPen(long laneId, QBrush &brush, QPen &pen);

    /*
    获取面域brush颜色
    @param connAreaId 连接区域ID
    @param color 输出颜色值
    @return 成功返回true，否则返回false
    */
    virtual bool connectorAreaBrushColor(long connAreaId, QColor& color);

    /*
    写标签，按照给定的属性名和字体大小（米）
    @param itemType 项目类型
    @param itemId 项目ID
    @param outPropName 输出属性名（枚举类型GraphicsItemPropName）
    @param outFontSize 输出字体大小（米）
    */
    virtual void labelNameAndFont(int itemType, long itemId, int &outPropName, qreal &outFontSize);

    /*
    判断是否绘制路段中心线
    @param linkId 路段ID
    @return 绘制返回true，否则返回false
    */
    virtual bool isDrawLinkCenterLine(long linkId);

    /*
    判断是否绘制路段四个拐角
    @param linkId 路段ID
    @return 绘制返回true，否则返回false
    */
    virtual bool isDrawLinkCorner(long linkId);

    /*
    判断是否绘制车道中心线
    @param laneId 车道ID
    @return 绘制返回true，否则返回false
    */
    virtual bool isDrawLaneCenterLine(long laneId);

    /*
    QGraphicsView的keyPressEvent事件后的处理
    @param event 键盘事件指针
    */
    virtual void afterViewKeyPressEvent(QKeyEvent* event);

    /*
    QGraphicsView的keyReleaseEvent事件后的处理
    @param event 键盘事件指针
    */
    virtual void afterViewKeyReleaseEvent(QKeyEvent *event);

    /*
    QGraphicsView的mouseDoubleClickEvent事件后的处理
    @param event 鼠标事件指针
    */
    virtual void afterViewMouseDoubleClickEvent(QMouseEvent * event);

    /*
    QGraphicsView的mouseMoveEvent事件后的处理
    @param event 鼠标事件指针
    */
    virtual void afterViewMouseMoveEvent(QMouseEvent *event);

    /*
    QGraphicsView的mousePressEvent事件后的处理
    @param event 鼠标事件指针
    */
    virtual void afterViewMousePressEvent(QMouseEvent *event);

    /*
    QGraphicsView的mouseReleaseEvent事件后的处理
    @param event 鼠标事件指针
    */
    virtual void afterViewMouseReleaseEvent(QMouseEvent *event);

    /*
    QGraphicsView的resizeEvent事件后的处理
    @param event 调整大小事件指针
    */
    virtual void afterViewResizeEvent(QResizeEvent *event);

    /*
    鼠标滚动事件响应
    @param event 滚轮事件指针
    */
    virtual void afterViewWheelEvent(QWheelEvent *event);

    /*
    滚动条移动事件后的处理
    @param dx 水平移动距离
    @param dy 垂直移动距离
    */
    virtual void afterViewScrollContentsBy(int dx, int dy);

    /*
    移动路段断点后调用
    @param pILink 路段接口指针
    @param vertexIndex 断点序号
    @param pressPoint 断点移动前位置
    @param releasePoint 断点移动后位置
    */
    virtual void afterLinkVertexMove(ILink* pILink, int vertexIndex, QPointF pressPoint, QPointF releasePoint);
};

#endif