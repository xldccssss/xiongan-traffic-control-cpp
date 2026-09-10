/********************************************************************************
* TESS NG路网层接口，此接口由TESS NG实现，用户可以通过此接口访问路网，获取路网数据，创建路段、发车点等
*********************************************************************************/

#ifndef NETINTERFACE_H
#define NETINTERFACE_H

#include <QObject>

#include <QGraphicsScene>
#include <QPointF>
#include <QList>
#include <QMap>

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"
#include "_tables.h"
#include "IPedestrianType.h"
#include "UnitChange.h"
#include "ITrafficLightMainline.h"

namespace Online {
	struct CrossPoint;
}

class CustomerNet;
class IRoadNet;
class ISection;
class ILink;
class Link;
class IConnector;
class Connector;
class IConnectorArea;
class IDispatchPoint;
class IDecisionPoint;
class CustomerGraphicsObject;
class IBusLine;
class IBusStation;
class IBusStationLine;
class IRouting;
class ISignalLamp;
class ISignalPhase;
//class ISignalGroup;
class IVehicleDrivInfoCollector;
class IVehicleQueueCounter;
class IVehicleTravelDetector;
class ILaneConnector;
class ILane;
class IGuidArrow;
class IRoadWorkZone;
class IAccidentZone;
class ILimitedZone;
class IReduceSpeedArea;
class IReconstruction;
class IPedestrianRegion;
class IPedestrianRectRegion;
class IPedestrianEllipseRegion;
class IPedestrianTriangleRegion;
class IPedestrianFanShapeRegion;
class IPedestrianPolygonRegion;
class IPedestrianSideWalkRegion;
class IPedestrianCrossWalkRegion;
class IPedestrianStairRegion;
class IPedestrianPathPoint;
class IPedestrianPath;
class ICrosswalkSignalLamp;

class ISignalController;
class ISignalPlan;
class IJunction;
class ITollLane;
class ITollPoint;
class ITollRouting;
class ITollDecisionPoint;
class IParkingStall;
class IParkingRegion;
class IParkingRouting;
class IParkingDecisionPoint;
class IEvaluateArea;


/*
访问、控制路网的接口
提供路网文件管理、路网元素查询、创建和编辑等功能
*/
class TESSINTERFACES_EXPORT NetInterface
{
public:
	virtual ~NetInterface() = default;

	/*
	打开路网文件
	@param filePath 路网文件路径
	*/
	virtual void openNetFle(QString filePath);

	/*
	从专业数据库加载路网
	@param netId 路网ID
	*/
	virtual void openNetByNetId(long netId);

	/*
	保存路网
	@return 保存成功返回true，否则返回false
	*/
	virtual bool saveRoadNet();

	/*
	获取路网文件路径
	如果是专业数据保存的路网，返回的是路网ID
	@return 返回路网文件路径或路网ID
	*/
	virtual QString netFilePath();

	/*
	获取路网对象
	@return 返回IRoadNet接口指针
	*/
	virtual IRoadNet* roadNet();

	/*
	获取路段面积
	@return 返回路段面积（平方米）
	*/
	virtual qreal roadNetArea();

	/*
	获取场景
	@return 返回QGraphicsScene指针
	*/
	virtual QGraphicsScene* graphicsScene();

	/*
	获取视图
	@return 返回QGraphicsView指针
	*/
	virtual QGraphicsView* graphicsView();

	/*
	获取场景中的比例尺
	@return 返回场景比例尺
	*/
	virtual qreal sceneScale();

	/*
	设置场景大小
	@param w 宽度
	@param h 高度
	@param unit 单位类型，默认为默认单位
	*/
	virtual void setSceneSize(qreal w, qreal h, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取场景宽度
	@param unit 单位类型，默认为默认单位
	@return 返回场景宽度
	*/
	virtual qreal sceneWidth(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取场景高度
	@param unit 单位类型，默认为默认单位
	@return 返回场景高度
	*/
	virtual qreal sceneHeigth(UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取背景图
	@return 返回背景图数据
	*/
	virtual QByteArray backgroundMap();

	/*
	设置路网像素比
	@param newRatio 新的像素比
	*/
	virtual void setPixelRatio(qreal newRatio);

	/*
	设置是否显示背景图
	@param bVisible true-显示背景图，false-隐藏背景图
	*/
	virtual void setBackgroundMapVisible(bool bVisible);

	/*
	设置是否显示网格线
	@param bVisible true-显示网格线，false-隐藏网格线
	*/
	virtual void setNetGridVisible(bool bVisible);

	/*
	开始数据导入（与endImportData()配合使用，可提高效率）
	此方法在开始导入数据前调用
	*/
	virtual void beginImportData();

	/*
	结束数据导入（与beginImportData()配合使用）
	此方法在完成导入数据后调用
	*/
	virtual void endImportData();

	/*
	获取所有Section
	@return 返回所有ISection接口的列表
	*/
	virtual QList<ISection*> sections();

	/*
	获取路段ID集
	@return 返回所有路段ID的列表
	*/
	virtual QList<long> linkIds();

	/*
	获取路段数
	@return 返回路段总数
	*/
	virtual int linkCount();

	/*
	获取路段集
	@return 返回所有ILink接口的列表
	*/
	virtual QList<ILink*> links();

	/*
	获取车型组成集
	@return 返回所有车型组成的列表
	*/
	virtual QList<Online::VehicleCompositionList> vehicleCompositions();

	/*
	根据路段ID查找路段
	@param id 路段ID
	@return 返回ILink接口指针，未找到返回nullptr
	*/
	virtual ILink* findLink(long id);

	/*
	根据车道ID查找车道
	@param id 车道ID
	@return 返回ILane接口指针，未找到返回nullptr
	*/
	virtual ILane* findLane(long id);

	/*
	根据车道连接ID查找车道连接
	@param id 车道连接ID
	@return 返回ILaneConnector接口指针，未找到返回nullptr
	*/
	virtual ILaneConnector* findLaneConnector(long id);

	/*
	获取连接段ID集
	@return 返回所有连接段ID的列表
	*/
	virtual QList<long> connectorIds();

	/*
	获取连接段数
	@return 返回连接段总数
	*/
	virtual int connectorCount();

	/*
	获取连接段集
	@return 返回所有IConnector接口的列表
	*/
	virtual QList<IConnector*> connectors();

	/*
	根据连接段ID查找连接段
	@param id 连接段ID
	@return 返回IConnector接口指针，未找到返回nullptr
	*/
	virtual IConnector* findConnector(long id);

	/*
	根据面域ID查找面域
	@param id 面域ID
	@return 返回IConnectorArea接口指针，未找到返回nullptr
	*/
	virtual IConnectorArea* findConnectorArea(long id);

	/*
	根据起始路段ID及目标路段ID查找连接段
	@param fromLinkId 起始路段ID
	@param toLinkId 目标路段ID
	@return 返回IConnector接口指针，未找到返回nullptr
	*/
	virtual IConnector* findConnectorByLinkIds(long fromLinkId, long toLinkId);

	/*
	根据起始车道ID及目标车道ID查找车道连接
	@param fromLaneId 起始车道ID
	@param toLaneId 目标车道ID
	@return 返回ILaneConnector接口指针，未找到返回nullptr
	*/
	virtual ILaneConnector* findLaneConnector(long fromLaneId, long toLaneId);

	/*
	获取导向箭头数
	@return 返回导向箭头总数
	*/
	virtual int guidArrowCount();

	/*
	获取导向箭头ID集
	@return 返回所有导向箭头ID的列表
	*/
	virtual QList<long> guidArrowIds();

	/*
	获取导向箭头集
	@return 返回所有IGuidArrow接口的列表
	*/
	virtual QList<IGuidArrow*> guidArrows();

	/*
	根据ID查询导向箭头
	@param id 导向箭头ID
	@return 返回IGuidArrow接口指针，未找到返回nullptr
	*/
	virtual IGuidArrow* findGuidArrow(long id);

	/*
	获取信号灯数
	@return 返回信号灯总数
	*/
	virtual int signalLampCount();

	/*
	获取信号灯ID集
	@return 返回所有信号灯ID的列表
	*/
	virtual QList<long> signalLampIds();

	/*
	获取信号灯集
	@return 返回所有ISignalLamp接口的列表
	*/
	virtual QList<ISignalLamp*> signalLamps();

	/*
	根据信号灯ID查找信号灯
	@param id 信号灯ID
	@return 返回ISignalLamp接口指针，未找到返回nullptr
	*/
	virtual ISignalLamp* findSignalLamp(long id);

	/*
	根据信号相位ID查找信号相位
	@param id 信号相位ID
	@return 返回ISignalPhase接口指针，未找到返回nullptr
	*/
	virtual ISignalPhase* findSignalPhase(long id);

	////信号灯组数
	//virtual int signalGroupCount();
	////信号灯组ID集
	//virtual QList<long> signalGroupIds();
	//信号灯组集
	//virtual QList<ISignalGroup*> signalGroups();
	//根据信号灯组ID查找信号灯组
	//virtual ISignalGroup* findSignalGroup(long id);

	/*
	获取信号机数
	@return 返回信号机总数
	*/
	virtual int signalControllerCount();

	/*
	获取信号机ID集
	@return 返回所有信号机ID的列表
	*/
	virtual QList<long> signalControllerIds();

	/*
	获取信号机集
	@return 返回所有ISignalController接口的列表
	*/
	virtual QList<ISignalController*> signalControllers();

	/*
	根据ID查询信号机
	@param id 信号机ID
	@return 返回ISignalController接口指针，未找到返回nullptr
	*/
	virtual ISignalController* findSignalControllerById(long id);

	/*
	根据名称查询信号机（如果同名返回第一个）
	@param name 信号机名称
	@return 返回ISignalController接口指针，未找到返回nullptr
	*/
	virtual ISignalController* findSignalControllerByName(QString name);

	/*
	获取信控方案组数
	@return 返回信控方案总数
	*/
	virtual int signalPlanCount();

	/*
	获取信控方案ID集
	@return 返回所有信控方案ID的列表
	*/
	virtual QList<long> signalPlanIds();

	/*
	获取信控方案集
	@return 返回所有ISignalPlan接口的列表
	*/
	virtual QList<ISignalPlan*> signalPlans();

	/*
	根据信控方案ID查找信控方案
	@param id 信控方案ID
	@return 返回ISignalPlan接口指针，未找到返回nullptr
	*/
	virtual ISignalPlan* findSignalPlanById(long id);

	/*
	根据信控方案名称查找信控方案
	@param name 信控方案名称
	@return 返回ISignalPlan接口指针，未找到返回nullptr
	*/
	virtual ISignalPlan* findSignalPlanByName(QString name);

	//干线相关接口

	/*
	查找所有干线ID列表
	@return 返回所有干线ID的列表
	*/
	virtual QList<int> ITrafficLightMainlineIds();

	/*
	查找所有干线
	@return 返回所有ITrafficLightMainline接口的列表
	*/
	virtual QList<ITrafficLightMainline*> ITrafficLightMainlines();

	/*
	根据ID查找干线
	@param id 干线ID
	@return 返回ITrafficLightMainline接口指针，未找到返回nullptr
	*/
	virtual ITrafficLightMainline* findITrafficLightMainlines(long id);

	/*
	创建干线
	@param id 干线ID
	@param name 干线名称
	@param lMainlines 干线列表
	@param info 输出参数，返回创建结果信息
	@return 返回创建的ITrafficLightMainline接口指针
	*/
	virtual ITrafficLightMainline* createITrafficLightMainline(int id, QString name, QList<Mainline> lMainlines, QString& info);

	/*
	删除干线
	@param id 干线ID
	@return 删除成功返回true，否则返回false
	*/
	virtual bool delTrafficLightMainline(int id);

	/*
	单点信控优化
	@param pISignalPlan 信控方案
	@param minCycleTime 最小周期时间
	@param maxCycleTime 最大周期时间
	@param LaneHourlyFlow 车道小时流量列表
	@param caleType 计算类型
	@param info 输出参数，返回优化结果信息
	@return 返回优化后的信控方案
	*/
	virtual ISignalPlan* caleSignalControlOptimization(ISignalPlan* pISignalPlan, int minCycleTime, int maxCycleTime, QList<int> LaneHourlyFlow, int caleType, QString& info);

	/*
	干线优化
	@param MainlineId 干线ID
	@param info 输出参数，返回优化结果信息
	@param c_min 最小周期
	@param c_max 最大周期
	@param leDownRatioUp 绿波带宽度上限比例
	@param directionType 方向类型
	@param lspeeds1 方向1车速列表
	@param lspeeds2 方向2车速列表
	@return 优化成功返回true，否则返回false
	*/
	virtual bool arterialTrafficSignalOptimization(int MainlineId, QString& info, double c_min, double c_max, double leDownRatioUp, int directionType, QList<double> lspeeds1, QList<double> lspeeds2);


	//发车点相关接口

	/*
	获取发车点集
	@return 返回所有IDispatchPoint接口的列表
	*/
	virtual QList<IDispatchPoint*> dispatchPoints();

	/*
	根据发车点ID查找发车点
	@param id 发车点ID
	@return 返回IDispatchPoint接口指针，未找到返回nullptr
	*/
	virtual IDispatchPoint* findDispatchPoint(long id);

	//决策点相关接口

	/*
	获取决策点集
	@return 返回所有IDecisionPoint接口的列表
	*/
	virtual QList<IDecisionPoint*> decisionPoints();

	/*
	根据决策点ID查找决策点
	@param id 决策点ID
	@return 返回IDecisionPoint接口指针，未找到返回nullptr
	*/
	virtual IDecisionPoint* findDecisionPoint(long id);

	//车辆检测器相关接口

	/*
	获取所有车辆检测器
	@return 返回所有IVehicleDrivInfoCollector接口的列表
	*/
	virtual QList<IVehicleDrivInfoCollector*> vehiInfoCollectors();

	/*
	根据ID查询车辆检测器
	@param id 检测器ID
	@return 返回IVehicleDrivInfoCollector接口指针，未找到返回nullptr
	*/
	virtual IVehicleDrivInfoCollector* findVehiInfoCollector(long id);

	//排队计数器相关接口

	/*
	获取所有排队计数器
	@return 返回所有IVehicleQueueCounter接口的列表
	*/
	virtual QList<IVehicleQueueCounter*> vehiQueueCounters();

	/*
	根据ID查询车辆排队计数器
	@param id 排队计数器ID
	@return 返回IVehicleQueueCounter接口指针，未找到返回nullptr
	*/
	virtual IVehicleQueueCounter* findVehiQueueCounter(long id);

	//行程时间检测器相关接口

	/*
	获取所有车辆行程时间检测器
	返回列表中的每一个元素是一对行程时间检测器的起始检测器
	@return 返回所有IVehicleTravelDetector接口的列表
	*/
	virtual QList<IVehicleTravelDetector*> vehiTravelDetectors();

	/*
	根据ID查询车辆行程时间检测器
	返回一对行程时间检测器中起始检测器
	@param id 检测器ID
	@return 返回IVehicleTravelDetector接口指针，未找到返回nullptr
	*/
	virtual IVehicleTravelDetector* findVehiTravelDetector(long id);

	/*
	根据路径ID查找路径
	@param id 路径ID
	@return 返回IRouting接口指针，未找到返回nullptr
	*/
	virtual IRouting* findRouting(long id);

	//公交线路相关接口

	/*
	获取公交线路集
	@return 返回所有IBusLine接口的列表
	*/
	virtual QList<IBusLine*> buslines();

	/*
	根据公交线路ID查询公交线路
	@param buslineId 公交线路ID
	@return 返回IBusLine接口指针，未找到返回nullptr
	*/
	virtual IBusLine* findBusline(long buslineId);

	/*
	根据公交线路起始路段ID查询公交线路
	@param linkId 起始路段ID
	@return 返回IBusLine接口指针，未找到返回nullptr
	*/
	virtual IBusLine* findBuslineByFirstLinkId(long linkId);

	//公交站点相关接口

	/*
	获取公交站点集
	@return 返回所有IBusStation接口的列表
	*/
	virtual QList<IBusStation*> busStations();

	/*
	根据公交站点ID查询公交站点
	@param stationId 公交站点ID
	@return 返回IBusStation接口指针，未找到返回nullptr
	*/
	virtual IBusStation* findBusStation(long stationId);

	/*
	根据公交站点ID查询相关BusLineStation
	@param stationId 公交站点ID
	@return 返回IBusStationLine接口列表
	*/
	virtual QList<IBusStationLine*> findBusStationLineByStationId(long stationId);

	/*
	获取面域集
	@return 返回所有IConnectorArea接口的列表
	*/
	virtual QList<IConnectorArea*> allConnectorArea();

	/*
	获取车道中心线断点集
	@param laneId 车道ID
	@param unit 单位类型，默认为默认单位
	@return 返回车道中心线断点列表
	*/
	virtual QList<QPointF> laneCenterPoints(long laneId, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取路段中心线断点集
	@param linkId 路段ID
	@param unit 单位类型，默认为默认单位
	@return 返回路段中心线断点列表
	*/
	virtual QList<QPointF> linkCenterPoints(long linkId, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	判断路段去向是否进入交叉口
	以面域是否存在多连接段以及当前路段与后续路段之间的角度为依据
	@param linkId 路段ID
	@return 如果进入交叉口返回true，否则返回false
	*/
	virtual bool judgeLinkToCross(long linkId);

	/*
	获取当前车道连接穿过其它车道连接形成的交叉点列表
	@param pLaneConnector 车道连接接口指针
	@return 返回交叉点列表
	*/
	virtual QList<Online::CrossPoint> crossPoints(ILaneConnector* pLaneConnector);

	/*
	根据路网元素名获取自增ID
	@param name 元素名称
	@return 返回自增ID
	*/
	virtual long getIDByItemName(QString name);

	/*
	获取路网基本属性
	@return 返回IRoadNet接口指针
	*/
	virtual IRoadNet* netAttrs();

	/*
	设置路网基本属性
	@param name 路网名称
	@param sourceType 数据来源分类，默认为"TESSNG"
	@param centerPoint 中心点坐标，默认为(0, 0)，用户也可以将中心点坐标保存到otherAttrsJson字段里
	@param backgroundUrl 背景图URL
	@param otherAttrsJson 保存在json对象中的其它属性
	@param unit 单位类型，默认为默认单位
	@return 返回IRoadNet接口指针
	*/
	virtual IRoadNet* setNetAttrs(QString name, QString sourceType = QString("TESSNG"), QPointF centerPoint = QPointF(), QString backgroundUrl = QString(), QJsonObject otherAttrsJson = QJsonObject(), UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建空白路网
	@param filePath 空白路网全路径名
	@param dbver 数据库版本，默认为5
	@return 创建成功返回true，否则返回false
	*/
	virtual bool createEmptyNetFile(QString filePath, long dbver = 5);

	/*
	创建路段（二维）
	@param lCenterPoint 路段中心线断点列表
	@param laneCount 车道数
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLink(QList<QPointF> lCenterPoint, int laneCount, QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建路段（三维）
	@param lCenterV3 路段中心线三维坐标列表
	@param laneCount 车道数
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLink3D(QList<QVector3D> lCenterV3, int laneCount, QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建路段（指定车道宽度）
	@param lCenterPoint 路段中心线断点列表
	@param lLaneWidth 车道宽度列表
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLinkWithLaneWidth(QList<QPointF> lCenterPoint, QList<qreal> lLaneWidth, QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建路段（指定车道宽度，OSM用）
	@param lCenterPoint 路段中心线断点列表
	@param lLaneWidth 车道宽度列表
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLinkWithLanesWidth(QList<QPointF> lCenterPoint, QList<qreal> lLaneWidth, QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建路段（三维，指定车道宽度）
	@param lCenterV3 路段中心线三维坐标列表
	@param lLaneWidth 车道宽度列表
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLink3DWithLanesWidth(QList<QVector3D> lCenterV3, QList<qreal> lLaneWidth, QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建路段（三维，指定车道宽度）
	@param lCenterV3 路段中心线三维坐标列表
	@param lLaneWidth 车道宽度列表
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLink3DWithLaneWidth(QList<QVector3D> lCenterV3, QList<qreal> lLaneWidth, QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建路段（三维，指定车道点集）
	@param lCenterLineV3 路段中心点集（对应TESSNG路段中心点）
	@param lanesWithPoints 车道点集的集合
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLink3DWithLanePoints(QList<QVector3D> lCenterLineV3, QList<QMap<QString, QList<QVector3D>>> lanesWithPoints, QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建路段（三维，指定车道点集和属性）
	@param lCenterLineV3 路段中心点集（对应TESSNG路段中心点）
	@param lanesWithPoints 车道点集的集合
	@param lLaneType 车道类型集
	@param lAttr 车道附加属性集，默认为空列表
	@param linkName 路段名称，默认为空（使用路段ID）
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILink接口指针
	*/
	virtual ILink* createLink3DWithLanePointsAndAttrs(QList<QVector3D> lCenterLineV3, QList<QMap<QString, QList<QVector3D>>> lanesWithPoints, QList<QString> lLaneType, QList<QJsonObject> lAttr = QList<QJsonObject>(), QString linkName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除路段
	从场景中移除pLink，但不从路网文件或数据库中删除，保存路网后才会从路网文件中删除
	@param pLink 要移除的路段接口指针
	*/
	virtual void removeLink(ILink* pLink);

	/*
	更新路段
	@param link 路段数据
	@param lLane 车道数据列表，默认为空
	@param lPoint 断点列表，默认为空
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回更新后的路段接口指针
	*/
	virtual ILink* updateLink(_Link link, QList<_Lane> lLane = QList<_Lane>(), QList<QPointF> lPoint = QList<QPointF>(), UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新路段断点高程
	车道断点高程也被更新，参数lV3z是断点高程列表，长度必须与路段断点数相同
	@param pILink 路段接口指针
	@param lV3z 断点高程列表
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void updateLinkV3z(ILink* pILink, QList<qreal> lV3z, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新路段断点集（三维）
	车道断点将被自动计算，更新后的断点数可以与原先断点数不同
	@param pILink 路段接口指针
	@param lV3d 三维断点列表
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void updateLink3DWithPoints(ILink* pILink, QList<QVector3D> lV3d, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新路段及车道断点集（三维）
	更新后的路段断点数与车道的断点数必须一致，但可以与原先断点数不同
	@param pILink 路段接口指针
	@param lCenterLineV3 路段中心线三维坐标列表
	@param lanesWithPoints 车道点集的集合
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void updateLinkAndLane3DWithPoints(ILink* pILink, QList<QVector3D> lCenterLineV3, QList<QMap<QString, QList<QVector3D>>> lanesWithPoints, UnitOfMeasure unit = UnitOfMeasure::Default);
	/*
	创建连接段
	@param fromLinkId 起始路段ID
	@param toLinkId 目标路段ID
	@param lFromLaneNumber 起始路段车道序号列表（最外侧为1）
	@param lToLaneNumber 目标路段车道序号列表（最外侧为1）
	@param connName 连接段名称，默认为空
	@param bAddToScene 是否添加到场景，默认为true
	@return 返回创建的IConnector接口指针
	*/
	virtual IConnector* createConnector(long fromLinkId, long toLinkId, QList<int> lFromLaneNumber, QList<int> lToLaneNumber, QString connName = QString(), bool bAddToScene = true);

	/*
	创建连接段（三维，指定断点）
	创建连接段后将laneConnector中自动计算的断点集用参数laneConnectorWithPoints断点替换
	@param fromLinkId 起始路段ID
	@param toLinkId 目标路段ID
	@param lFromLaneNumber 起始路段车道序号列表（最外侧为1）
	@param lToLaneNumber 目标路段车道序号列表（最外侧为1）
	@param laneConnectorWithPoints 车道连接断点集
	@param connName 连接段名称，默认为空
	@param bAddToScene 是否添加到场景，默认为true
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IConnector接口指针
	*/
	virtual IConnector* createConnector3DWithPoints(long fromLinkId, long toLinkId, QList<int> lFromLaneNumber, QList<int> lToLaneNumber, QList<QMap<QString, QList<QVector3D>>> laneConnectorWithPoints, QString connName = QString(), bool bAddToScene = true, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除连接段
	从场景中移除pConnector，但不从路网文件或数据库中删除，保存路网后才会从路网文件中删除
	@param pConnector 要移除的连接段接口指针
	*/
	virtual void removeConnector(IConnector* pConnector);

	/*
	更新连接段
	@param connector 连接段数据
	@return 返回更新后的连接段接口指针
	*/
	virtual IConnector* updateConnector(_Connector connector);

	/*
	创建车道连接
	@param pIConnector 连接段接口指针
	@param fromLaneId 上游车道ID
	@param toLaneId 下游车道ID
	@return 返回创建的ILaneConnector接口指针
	*/
	virtual ILaneConnector* createLaneConnector(IConnector* pIConnector, long fromLaneId, long toLaneId);

	/*
	移除车道连接
	@param pILC 要移除的车道连接接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeLaneConnector(ILaneConnector* pILC);

	/*
	创建导向箭头
	@param pLane 车道接口指针
	@param length 箭头长度
	@param distToTerminal 到车道终点的距离
	@param arrowType 箭头类型
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IGuidArrow接口指针
	*/
	virtual IGuidArrow* createGuidArrow(ILane* pLane, qreal length, qreal distToTerminal, Online::GuideArrowType arrowType, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	删除导向箭头
	@param pArrow 要删除的导向箭头接口指针
	*/
	virtual void removeGuidArrow(IGuidArrow* pArrow);

	/*
	创建发车点
	@param pLink 路段接口指针
	@param dpName 发车点名称，默认为空
	@param bAddToScene 是否添加到场景，默认为true
	@return 返回创建的IDispatchPoint接口指针
	*/
	virtual IDispatchPoint* createDispatchPoint(ILink* pLink, QString dpName = QString(), bool bAddToScene = true);

	/*
	删除发车点
	@param pDispPoint 要删除的发车点接口指针
	@return 删除成功返回true，否则返回false
	*/
	virtual bool removeDispatchPoint(IDispatchPoint* pDispPoint);
	/*
	获取车辆类型列表
	@return 返回所有车辆类型的列表
	*/
	virtual QList<_VehicleType> vehicleTypes();

	/*
	创建车型
	@param _vt 车型数据
	@return 创建成功返回true，否则返回false
	*/
	virtual bool createVehicleType(_VehicleType _vt);

	/*
	创建车型组成
	@param name 车型组成名称
	@param lVehiComp 车型组成明细列表
	@return 返回创建的车型组成ID
	*/
	virtual long createVehicleComposition(QString name, QList<Online::VehiComposition> lVehiComp);

	/*
	更新车型组成
	@param vehiCompId 车型组成ID
	@param lVehiComp 更新后的车型组成明细列表
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateVehicleComposition(long vehiCompId, QList<Online::VehiComposition> lVehiComp);

	/*
	移除车型组成
	@param vehiCompId 车型组成ID
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeVehicleComposition(long vehiCompId);

	/*
	创建决策点
	@param pLink 路段接口指针
	@param distance 距路段起点的距离
	@param name 决策点名称，默认为空
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IDecisionPoint接口指针
	*/
	virtual IDecisionPoint* createDecisionPoint(ILink* pLink, qreal distance, QString name = QString(), UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除决策点
	@param pDeciPoint 要移除的决策点接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeDecisionPoint(IDecisionPoint* pDeciPoint);

	/*
	创建决策路径
	@param pDeciPoint 决策点接口指针
	@param lILink 路段列表
	@return 返回创建的IRouting接口指针
	*/
	virtual IRouting* createDeciRouting(IDecisionPoint* pDeciPoint, QList<ILink*> lILink);

	/*
	删除决策路径
	@param pDeciPoint 决策点接口指针
	@param pRouting 要删除的路径接口指针
	@return 删除成功返回true，否则返回false
	*/
	virtual bool removeDeciRouting(IDecisionPoint* pDeciPoint, IRouting* pRouting);

	/*
	更新决策点及其各路径不同时间段流量比
	@param deciPoint 决策点数据
	@param lFlowRatio 流量比列表，默认为空
	@return 返回更新后的决策点接口指针
	*/
	virtual IDecisionPoint* updateDecipointPoint(_DecisionPoint deciPoint, QList<_RoutingFLowRatio> lFlowRatio = QList<_RoutingFLowRatio>());

	/*
	更新决策点下的决策路径
	@param pIRouting 路径接口指针
	@param lILink 更新后的路段列表
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateRouting(IRouting* pIRouting, QList<ILink*> lILink);

	/*
	更新决策路径上一个连接段的车道连接
	@param pRouting 路径接口指针
	@param pConnector 连接段接口指针
	@param lFromLaneId 连接段上游车道ID列表
	@param lToLaneId 连接段下游车道ID列表
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateRoutingLaneConnector(IRouting* pRouting, IConnector* pConnector, QList<long> lFromLaneId, QList<long> lToLaneId);
	/*
	在路段的车道上创建车辆采集器
	@param pLane 车道接口指针
	@param dist 距车道起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IVehicleDrivInfoCollector接口指针
	*/
	virtual IVehicleDrivInfoCollector* createVehiCollectorOnLink(ILane* pLane, qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	在连接段的车道连接上创建车辆采集器
	@param pLaneConnector 车道连接接口指针
	@param dist 距车道连接起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IVehicleDrivInfoCollector接口指针
	*/
	virtual IVehicleDrivInfoCollector* createVehiCollectorOnConnector(ILaneConnector* pLaneConnector, qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新采集器
	@param collecter 采集器数据
	@return 返回更新后的采集器接口指针
	*/
	virtual IVehicleDrivInfoCollector* updateVehiCollector(_VehicleDrivInfoCollector collecter);

	/*
	移除车辆检测器
	@param pCollector 要移除的检测器接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeVehiCollector(IVehicleDrivInfoCollector* pCollector);

	/*
	在路段的车道上创建车辆排队计数器
	@param pLane 车道接口指针
	@param dist 距车道起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IVehicleQueueCounter接口指针
	*/
	virtual IVehicleQueueCounter* createVehiQueueCounterOnLink(ILane* pLane, qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	在连接段的车道连接上创建车辆排队计数器
	@param pLaneConnector 车道连接接口指针
	@param dist 距车道连接起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IVehicleQueueCounter接口指针
	*/
	virtual IVehicleQueueCounter* createVehiQueueCounterOnConnector(ILaneConnector* pLaneConnector, qreal dist, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新排队计数器
	@param counter 排队计数器数据
	@return 返回更新后的排队计数器接口指针
	*/
	virtual IVehicleQueueCounter* updateVehiQueueCounter(_VehicleQueueCounter counter);

	/*
	移除排队计数器
	@param pCounter 要移除的排队计数器接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeVehiQueueCounter(IVehicleQueueCounter* pCounter);

	/*
	创建行程时间检测器（路段到路段）
	起点和终点都在路段上
	@param pStartLink 起始路段接口指针
	@param pEndLink 终点路段接口指针
	@param dist1 检测器起点距起始路段起点的距离
	@param dist2 检测器终点距终点路段起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回行程时间检测器列表
	*/
	virtual QList<IVehicleTravelDetector*> createVehicleTravelDetector_link2link(ILink* pStartLink, ILink* pEndLink, qreal dist1, qreal dist2, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建行程时间检测器（路段到连接段）
	起点在路段上，终点在连接段的车道连接上
	@param pStartLink 起始路段接口指针
	@param pEndLaneConnector 终点车道连接接口指针
	@param dist1 检测器起点距起始路段起点的距离
	@param dist2 检测器终点距车道连接起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回行程时间检测器列表
	*/
	virtual QList<IVehicleTravelDetector*> createVehicleTravelDetector_link2conn(ILink* pStartLink, ILaneConnector* pEndLaneConnector, qreal dist1, qreal dist2, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建行程时间检测器（连接段到路段）
	起点在连接段的车道连接上，终点在路段上
	@param pStartLaneConnector 起始车道连接接口指针
	@param pEndLink 终点路段接口指针
	@param dist1 检测器起点距车道连接起点的距离
	@param dist2 检测器终点距路段起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回行程时间检测器列表
	*/
	virtual QList<IVehicleTravelDetector*> createVehicleTravelDetector_conn2link(ILaneConnector* pStartLaneConnector, ILink* pEndLink, qreal dist1, qreal dist2, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建行程时间检测器（连接段到连接段）
	起点和终点都在连接段的车道连接上
	@param pStartLaneConnector 起始车道连接接口指针
	@param pEndLaneConnector 终点车道连接接口指针
	@param dist1 检测器起点距起始车道连接起点的距离
	@param dist2 检测器终点距终点车道连接起点的距离
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回行程时间检测器列表
	*/
	virtual QList<IVehicleTravelDetector*> createVehicleTravelDetector_conn2conn(ILaneConnector* pStartLaneConnector, ILaneConnector* pEndLaneConnector, qreal dist1, qreal dist2, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除行程时间检测器
	@param pDetector 要移除的检测器接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeVehicleTravelDetector(IVehicleTravelDetector* pDetector);

	//创建信号灯组， 参数 name:灯组名称， period:周期，单位秒，fromTime:起始时间，单位秒，toTime:结束时间,单位秒
	//virtual ISignalGroup* createSignalGroup(QString name, int period, long fromTime, long toTime);
	//创建相位， 参数 pGroup：信号灯组， name：相位名称，lColor：相位灯色序列，新建相位排在已有相位序列的最后
	//virtual ISignalPhase* createSignalPhase(ISignalGroup* pGroup, QString name, QList<Online::ColorInterval> lColor); 
	

	/*
	创建相位（基于信控方案）
	新建相位排在已有相位序列的最后
	@param SignalPlan 信控方案接口指针
	@param name 相位名称
	@param lColor 相位灯色序列
	@return 返回创建的ISignalPhase接口指针
	*/
	virtual ISignalPhase* createSignalPlanSignalPhase(ISignalPlan* SignalPlan, QString name, QList<Online::ColorInterval> lColor);

	/*
	移除相位
	相位移除后，原相位序列自然重排
	@param pPlan 信控方案接口指针
	@param phaseId 要移除的相位ID
	*/
	virtual void removeSignalPhase(ISignalPlan* pPlan, long phaseId);
	/*
	创建信号灯（基于相位）
	@param pPhase 相位接口指针
	@param name 信号灯名称
	@param laneId 信号灯所在车道ID，或所在车道连接上游车道ID
	@param toLaneId 信号灯所在车道连接下游车道ID
	@param distance 信号灯距车道或车道连接起点的距离（像素）
	@return 返回创建的ISignalLamp接口指针
	*/
	virtual ISignalLamp* createSignalLamp(ISignalPhase* pPhase, QString name, long laneId, long toLaneId, qreal distance);

	/*
	移除信号灯
	@param pLamp 要移除的信号灯接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeSignalLamp(ISignalLamp* pLamp);

	/*
	创建信号灯（基于信号机）
	@param pTrafficLight 信号机接口指针
	@param name 信号灯名称
	@param laneId 信号灯所在车道ID，或所在车道连接上游车道ID
	@param toLaneId 信号灯所在车道连接下游车道ID
	@param distance 信号灯距车道或车道连接起点的距离（像素）
	@return 返回创建的ISignalLamp接口指针
	*/
	virtual ISignalLamp* createTrafficSignalLamp(ISignalController* pTrafficLight, QString name, long laneId, long toLaneId, qreal distance);

	/*
	创建信号机
	@param name 信号机名称
	@return 返回创建的ISignalController接口指针
	*/
	virtual ISignalController* createSignalController(QString name);

	/*
	移除信号机
	@param pTrafficLight 要移除的信号机接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeSignalController(ISignalController* pTrafficLight);

	/*
	创建信控方案
	@param pITrafficLight 信号机接口指针
	@param name 信控方案名称
	@param cycle 周期（秒）
	@param offset 偏移（秒）
	@param startTime 开始时间（秒）
	@param endTime 结束时间（秒）
	@return 返回创建的ISignalPlan接口指针
	*/
	virtual ISignalPlan* createSignalPlan(ISignalController* pITrafficLight, QString name, int cycle, int offset, int startTime, int endTime);

	/*
	移除信控方案
	@param pPlan 要移除的信控方案接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeSignalPlan(ISignalPlan* pPlan);

	/*
	移除相位
	@param pPhase 要移除的相位接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removePhase(ISignalPhase* pPhase);

	/*
	信号灯移除绑定的相位
	如果相位列表只存在一个相位则将关联的相位设置为null
	@param SignalPhaseId 相位ID
	@param signalLamp 信号灯接口指针
	*/
	virtual void removeSignalPhaseFromLamp(int SignalPhaseId, ISignalLamp* signalLamp);

	/*
	信号灯添加绑定的相位
	@param SignalPhaseId 相位ID
	@param signalLamp 信号灯接口指针
	*/
	virtual void addSignalPhaseToLamp(int SignalPhaseId, ISignalLamp* signalLamp);

	/*
	人行横道信号灯添加绑定的相位
	@param SignalPhaseId 相位ID
	@param signalLamp 人行横道信号灯接口指针
	*/
	virtual void addCrossWalkSignalPhaseToLamp(int SignalPhaseId, ICrosswalkSignalLamp* signalLamp);

	/*
	信号灯更换绑定的相位（不允许跨越信号机）
	@param pFromISignalPhase 原相位接口指针
	@param pToISignalPhase 目标相位接口指针
	@param signalLamp 信号灯接口指针
	*/
	virtual void transferSignalPhase(ISignalPhase* pFromISignalPhase, ISignalPhase* pToISignalPhase, ISignalLamp* signalLamp);

	/*
	创建公交线路
	lLink列表中相邻两路段可以是路网上相邻两路段，也可以不相邻，如果不相邻，TESSNG会在它们之间创建一条最短路径。
	如果lLink列表中相邻路段在路网上不相邻并且二者之间不存在最短路径，则相邻的第二条路段及后续路段无效。
	@param lLink 路段列表
	@return 返回创建的IBusLine接口指针
	*/
	virtual IBusLine* createBusLine(QList<ILink*> lLink);

	/*
	移除公交线路
	@param pBusLine 要移除的公交线路接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeBusLine(IBusLine* pBusLine);

	/*
	创建公交站点
	@param pLane 车道接口指针
	@param length 站点长度
	@param dist 站点起始点距车道起点的距离
	@param name 站点名称，默认为空
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IBusStation接口指针
	*/
	virtual IBusStation* createBusStation(ILane* pLane, qreal length, qreal dist, QString name = QString(), UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除公交站点
	@param pStation 要移除的公交站点接口指针
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removeBusStation(IBusStation* pStation);

	/*
	将公交站点关联到公交线路上
	@param pBusLine 公交线路接口指针
	@param pStation 公交站点接口指针
	@return 关联成功返回true，否则返回false
	*/
	virtual bool addBusStationToLine(IBusLine* pBusLine, IBusStation* pStation);

	/*
	将公交站点与公交线路的关联关系解除
	@param pBusLine 公交线路接口指针
	@param pStation 公交站点接口指针
	@return 解除成功返回true，否则返回false
	*/
	virtual bool removeBusStationFromLine(IBusLine* pBusLine, IBusStation* pStation);
	/*
	编辑公交站点基础信息
	@param pBusLine 公交线路接口指针
	@param pStation 公交站点接口指针
	@param parkingTime 基本停靠时间
	@param leavingPercent 下客百分比
	@param getOnTimePerson 上车时间（每人）
	@param getOutTimePerson 下车时间（每人）
	@return 编辑成功返回true，否则返回false
	*/
	virtual bool editBusStationBasicInformation(IBusLine* pBusLine, IBusStation* pStation, int parkingTime, double leavingPercent, double getOnTimePerson, double getOutTimePerson);

	/*
	编辑公交站点关联线路信息
	@param pBusLine 公交线路接口指针
	@param pStation 公交站点接口指针
	@param lPassengerArrivings 乘客到达信息列表
	@return 编辑成功返回true，否则返回false
	*/
	virtual bool editBusStation(IBusLine* pBusLine, IBusStation* pStation, QList<Online::PassengerArrivings> lPassengerArrivings);

	/*
	创建限速区
	@param param 限速区参数
	@return 返回创建的IReduceSpeedArea接口指针
	*/
	virtual IReduceSpeedArea* createReduceSpeedArea(Online::DynaReduceSpeedAreaParam param);

	/*
	移除限速区
	@param pIReduceSpeedArea 要移除的限速区接口指针
	*/
	virtual void removeReduceSpeedArea(IReduceSpeedArea* pIReduceSpeedArea);

	/*
	更新限速区
	@param param 更新后的限速区参数
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateReduceSpeedArea(Online::DynaReduceSpeedAreaParam param);

	/*
	获取所有限速区
	@return 返回所有IReduceSpeedArea接口的列表
	*/
	virtual QList<IReduceSpeedArea*> reduceSpeedAreas();

	/*
	查询指定ID的限速区
	@param id 限速区ID
	@return 返回IReduceSpeedArea接口指针，未找到返回nullptr
	*/
	virtual IReduceSpeedArea* findReduceSpeedArea(long id);

	/*
	初始化数据库序列
	@param schemaName 模式名称，默认为空
	@return 初始化成功返回true，否则返回false
	*/
	virtual bool initSequence(QString schemaName = QString());

	/*
	计算最短路径
	@param pFromLink 起始路段接口指针
	@param pToLink 目标路段接口指针
	@return 返回最短路径上的路段序列
	*/
	virtual IRouting* shortestRouting(ILink* pFromLink, ILink* pToLink);

	/*
	用连续通达的路段序列创建路径
	@param lILink 路段列表
	@return 返回创建的IRouting接口指针
	*/
	virtual IRouting* createRouting(QList<ILink*> lILink);

	/*
	场景角度转换到图元角度
	@param angle 场景角度
	@return 返回图元角度
	*/
	virtual qreal angleToItem(qreal angle);

	/*
	增加客户路网元素到场景
	@param pCustObj 客户路网元素接口指针
	*/
	virtual void addCustomerItem(CustomerGraphicsObject* pCustObj);

	/*
	从场景移除客户路网元素
	@param pCustObj 客户路网元素接口指针
	*/
	virtual void removeCustomerItem(CustomerGraphicsObject* pCustObj);

	/*
	路网的网格化
	@param width 单元格宽度，默认为25
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void buildNetGrid(qreal width = 25, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	根据点查询所在单元格所有Section
	@param point 查询点坐标
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回所在单元格内的ISection接口列表
	*/
	virtual QList<ISection*> findSectionOn1Cell(QPointF point, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	根据点查询最近4个单元格所有Section
	@param point 查询点坐标
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回最近4个单元格内的ISection接口列表
	*/
	virtual QList<ISection*> findSectionOn4Cell(QPointF point, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	根据点查询最近9个单元格所有Section
	@param point 查询点坐标
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回最近9个单元格内的ISection接口列表
	*/
	virtual QList<ISection*> findSectionOn9Cell(QPointF point, UnitOfMeasure unit = UnitOfMeasure::Default);
	/*
	根据点对Section列表中每个Section的所有LaneObject求最短距离
	返回Location列表，按最短距离排序（从小到大）
	@param point 查询点坐标
	@param lSection Section列表
	@param referDistance LaneObject上与point最近的点到LaneObject起点距离（大约数，用于提高计算效率），默认为0
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回Location列表
	*/
	virtual QList<Online::Location> locateOnSections(QPointF point, QList<ISection*> lSection, qreal referDistance = 0, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	在点周围若干个单元格里查询LaneObject
	cellCount：单元格数，小于1时默认为1，大于1小于4时默认为4，大于4时默认为9
	@param point 查询点坐标
	@param cellCount 单元格数，默认为1
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回Location列表
	*/
	virtual QList<Online::Location> locateOnCrid(QPointF point, int cellCount = 1, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取路网外围矩形（用于获取路网边界）
	@return 返回路网边界矩形
	*/
	virtual QRectF boundingRect();

	/*
	创建施工区
	@param param 施工区参数
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IRoadWorkZone接口指针
	*/
	virtual IRoadWorkZone* createRoadWorkZone(Online::DynaRoadWorkZoneParam param, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新施工区
	@param param 更新后的施工区参数
	@param unit 单位类型，默认为默认单位（像素）
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateRoadWorkZone(Online::DynaRoadWorkZoneParam param, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除施工区
	@param pIRoadWorkZone 要移除的施工区接口指针
	*/
	virtual void removeRoadWorkZone(IRoadWorkZone* pIRoadWorkZone);

	/*
	获取所有施工区
	@return 返回所有IRoadWorkZone接口的列表
	*/
	virtual QList<IRoadWorkZone*> roadWorkZones();

	/*
	根据ID查询施工区
	@param roadWorkZoneId 施工区ID
	@return 返回IRoadWorkZone接口指针，未找到返回nullptr
	*/
	virtual IRoadWorkZone* findRoadWorkZone(long roadWorkZoneId);

	/*
	创建事故区
	@param param 事故区参数
	@return 返回创建的IAccidentZone接口指针
	*/
	virtual IAccidentZone* createAccidentZone(Online::DynaAccidentZoneParam param);

	/*
	移除事故区
	@param pIAccidentZone 要移除的事故区接口指针
	*/
	virtual void removeAccidentZone(IAccidentZone* pIAccidentZone);

	/*
	更新事故区
	@param param 更新后的事故区参数
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateAccidentZone(Online::DynaAccidentZoneParam param);

	/*
	获取所有事故区
	@return 返回所有IAccidentZone接口的列表
	*/
	virtual QList<IAccidentZone*> accidentZones();

	/*
	根据ID查询事故区
	@param accidentZoneId 事故区ID
	@return 返回IAccidentZone接口指针，未找到返回nullptr
	*/
	virtual IAccidentZone* findAccidentZone(long accidentZoneId);

	/*
	创建限行区
	@param param 限行区参数
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的ILimitedZone接口指针
	*/
	virtual ILimitedZone* createLimitedZone(Online::DynaLimitedZoneParam param, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新限行区
	@param param 更新后的限行区参数
	@param unit 单位类型，默认为默认单位（像素）
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateLimitedZone(Online::DynaLimitedZoneParam param, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除限行区
	@param pILimitedZone 要移除的限行区接口指针
	*/
	virtual void removeLimitedZone(ILimitedZone* pILimitedZone);

	/*
	获取所有限行区
	@return 返回所有ILimitedZone接口的列表
	*/
	virtual QList<ILimitedZone*> limitedZones();

	/*
	根据ID查询限行区
	@param limitedZoneId 限行区ID
	@return 返回ILimitedZone接口指针，未找到返回nullptr
	*/
	virtual ILimitedZone* findLimitedZone(long limitedZoneId);

	/*
	移动路段及相关连接段
	@param lLink 要移动的路段列表
	@param offset 偏移量
	@param unit 单位类型，默认为默认单位（像素）
	*/
	virtual void moveLinks(QList<ILink*> lLink, QPointF offset, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	创建改扩建
	@param param 改扩建参数
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回创建的IReconstruction接口指针
	*/
	virtual IReconstruction* createReconstruction(Online::DynaReconstructionParam param, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	更新改扩建
	@param param 更新后的改扩建参数
	@param unit 单位类型，默认为默认单位（像素）
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateReconStruction(Online::DynaReconstructionParam param, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	移除改扩建
	@param pIReconstruction 要移除的改扩建接口指针
	*/
	virtual void removeReconstruction(IReconstruction* pIReconstruction);

	/*
	获取所有改扩建
	@return 返回所有IReconstruction接口的列表
	*/
	virtual QList<IReconstruction*> reconstructions();

	/*
	根据ID查询改扩建
	@param reconstructionId 改扩建ID
	@return 返回IReconstruction接口指针，未找到返回nullptr
	*/
	virtual IReconstruction* findReconstruction(long reconstructionId);

	/*
	重新计算保通开口长度
	@param param 改扩建参数
	@param unit 单位类型，默认为默认单位（像素）
	@return 返回保通开口长度
	*/
	virtual qreal reCalcPassagewayLength(Online::DynaReconstructionParam param, UnitOfMeasure unit = UnitOfMeasure::Default);


	//行人相关接口

	/*
	获取所有行人类型
	@return 返回所有行人类型的列表
	*/
	virtual QList<IPedestrianType> pedestrianTypes();

	/*
	获取所有行人组成
	@return 返回所有行人组成的列表
	*/
	virtual QList<Online::Pedestrian::PedestrianComposition> pedestrianCompositions();

	/*
	获取所有层级信息
	@return 返回所有层级信息的列表
	*/
	virtual QList<Online::Pedestrian::LayerInfo> layerInfos();

	/*
	获取所有行人面域
	@return 返回所有IPedestrianRegion接口的列表
	*/
	virtual QList<IPedestrianRegion*> pedestrianRegions();

	/*
	获取所有矩形面域
	@return 返回所有IPedestrianRectRegion接口的列表
	*/
	virtual QList<IPedestrianRectRegion*> pedestrianRectRegions();

	/*
	获取所有椭圆形面域
	@return 返回所有IPedestrianEllipseRegion接口的列表
	*/
	virtual QList<IPedestrianEllipseRegion*> pedestrianEllipseRegions();

	/*
	获取所有三角形面域
	@return 返回所有IPedestrianTriangleRegion接口的列表
	*/
	virtual QList<IPedestrianTriangleRegion*> pedestrianTriangleRegions();

	/*
	获取所有扇形面域
	@return 返回所有IPedestrianFanShapeRegion接口的列表
	*/
	virtual QList<IPedestrianFanShapeRegion*> pedestrianFanShapeRegions();

	/*
	获取所有多边形面域
	@return 返回所有IPedestrianPolygonRegion接口的列表
	*/
	virtual QList<IPedestrianPolygonRegion*> pedestrianPolygonRegions();

	/*
	获取所有人行道
	@return 返回所有IPedestrianSideWalkRegion接口的列表
	*/
	virtual QList<IPedestrianSideWalkRegion*> pedestrianSideWalkRegions();

	/*
	获取所有人行横道
	@return 返回所有IPedestrianCrossWalkRegion接口的列表
	*/
	virtual QList<IPedestrianCrossWalkRegion*> pedestrianCrossWalkRegions();
	/*
	获取所有楼梯
	@return 返回所有IPedestrianStairRegion接口的列表
	*/
	virtual QList<IPedestrianStairRegion*> pedestrianStairRegions();

	/*
	获取所有行人发生点
	@return 返回所有IPedestrianPathPoint接口的列表
	*/
	virtual QList<IPedestrianPathPoint*> pedestrianPathStartPoints();

	/*
	获取所有行人结束点
	@return 返回所有IPedestrianPathPoint接口的列表
	*/
	virtual QList<IPedestrianPathPoint*> pedestrianPathEndPoints();

	/*
	获取所有行人决策点
	@return 返回所有IPedestrianPathPoint接口的列表
	*/
	virtual QList<IPedestrianPathPoint*> pedestrianPathDecisionPoints();

	/*
	获取所有行人路径（包括局部路径）
	@return 返回所有IPedestrianPath接口的列表
	*/
	virtual QList<IPedestrianPath*> pedestrianPaths();

	/*
	获取所有人行横道红绿灯
	@return 返回所有ICrosswalkSignalLamp接口的列表
	*/
	virtual QList<ICrosswalkSignalLamp*> crosswalkSignalLamps();

	/*
	根据ID获取行人面域
	@param id 面域ID
	@return 返回IPedestrianRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianRegion* findPedestrianRegion(long id);

	/*
	根据ID获取矩形面域
	@param id 面域ID
	@return 返回IPedestrianRectRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianRectRegion* findPedestrianRectRegion(long id);

	/*
	根据ID获取椭圆形面域
	@param id 面域ID
	@return 返回IPedestrianEllipseRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianEllipseRegion* findPedestrianEllipseRegion(long id);

	/*
	根据ID获取三角形面域
	@param id 面域ID
	@return 返回IPedestrianTriangleRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianTriangleRegion* findPedestrianTriangleRegion(long id);

	/*
	根据ID获取扇形面域
	@param id 面域ID
	@return 返回IPedestrianFanShapeRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianFanShapeRegion* findPedestrianFanShapeRegion(long id);

	/*
	根据ID获取多边形面域
	@param id 面域ID
	@return 返回IPedestrianPolygonRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianPolygonRegion* findPedestrianPolygonRegion(long id);

	/*
	根据ID获取人行道
	@param id 人行道ID
	@return 返回IPedestrianSideWalkRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianSideWalkRegion* findPedestrianSideWalkRegion(long id);

	/*
	根据ID获取人行横道
	@param id 人行横道ID
	@return 返回IPedestrianCrossWalkRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianCrossWalkRegion* findPedestrianCrossWalkRegion(long id);

	/*
	根据ID获取楼梯
	@param id 楼梯ID
	@return 返回IPedestrianStairRegion接口指针，未找到返回nullptr
	*/
	virtual IPedestrianStairRegion* findPedestrianStairRegion(long id);

	/*
	根据ID获取行人发生点
	@param id 行人发生点ID
	@return 返回IPedestrianPathPoint接口指针，未找到返回nullptr
	*/
	virtual IPedestrianPathPoint* findPedestrianPathStartPoint(long id);

	/*
	根据ID获取行人结束点
	@param id 行人结束点ID
	@return 返回IPedestrianPathPoint接口指针，未找到返回nullptr
	*/
	virtual IPedestrianPathPoint* findPedestrianPathEndPoint(long id);

	/*
	根据ID获取行人决策点
	@param id 行人决策点ID
	@return 返回IPedestrianPathPoint接口指针，未找到返回nullptr
	*/
	virtual IPedestrianPathPoint* findPedestrianDecisionPoint(long id);

	/*
	根据ID获取行人路径中间点
	@param id 中间点ID
	@return 返回IPedestrianPathPoint接口指针，未找到返回nullptr
	*/
	virtual IPedestrianPathPoint* findPedestrianPathMiddlePoint(long id);

	/*
	根据ID获取行人路径（包括局部路径）
	@param id 路径ID
	@return 返回IPedestrianPath接口指针，未找到返回nullptr
	*/
	virtual IPedestrianPath* findPedestrianPath(long id);

	/*
	根据ID获取人行横道红绿灯
	@param id 红绿灯ID
	@return 返回ICrosswalkSignalLamp接口指针，未找到返回nullptr
	*/
	virtual ICrosswalkSignalLamp* findCrosswalkSignalLamp(long id);
	/*
	根据ID获取行人发生点配置信息
	@param id 行人发生点ID
	@return 返回行人发生点配置信息
	*/
	virtual Online::Pedestrian::PedestrianPathStartPointConfigInfo findPedestrianStartPointConfigInfo(long id);

	/*
	根据ID获取行人决策点配置信息
	@param id 行人决策点ID
	@return 返回行人决策点配置信息
	*/
	virtual Online::Pedestrian::PedestrianDecisionPointConfigInfo findPedestrianDecisionPointConfigInfo(long id);

	/*
	创建行人组成
	@param name 组成名称
	@param mpCompositionRatio 组成明细(key:行人类型编码,value:比重)
	@return 返回组成ID，创建失败返回-1
	*/
	virtual long createPedestrianComposition(QString name, QMap<int, qreal> mpCompositionRatio);

	/*
	更新行人组成
	@param compositionId 组成ID
	@param mpCompositionRatio 更新后的组成明细
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updatePedestrianComposition(long compositionId, QMap<int, qreal> mpCompositionRatio);

	/*
	移除行人组成
	@param compositionId 组成ID
	@return 移除成功返回true，否则返回false
	*/
	virtual bool removePedestrianComposition(long compositionId);

	/*
	新增层级
	@param name 层级名称
	@param height 层级高度
	@param visible 是否可见
	@param locked 是否锁定（锁定后面域不可修改）
	@return 返回新增的层级信息
	*/
	virtual Online::Pedestrian::LayerInfo addLayerInfo(QString name, qreal height, bool visible, bool locked);

	/*
	删除某个层级（会删除层级中的所有元素）
	@param layerId 层级ID
	*/
	virtual void removeLayerInfo(long layerId);

	/*
	更新层级信息
	@param layerId 层级ID
	@param name 层级名称
	@param height 层级高度
	@param visible 是否可见
	@param locked 是否锁定（锁定后面域不可修改）
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateLayerInfo(long layerId, QString name, qreal height, bool visible, bool locked);

	/*
	更新行人发生点配置信息
	@param info 配置信息
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updatePedestrianStartPointConfigInfo(Online::Pedestrian::PedestrianPathStartPointConfigInfo info);

	/*
	更新行人决策点配置信息
	@param info 配置信息
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updatePedestrianDecisionPointConfigInfo(Online::Pedestrian::PedestrianDecisionPointConfigInfo info);

	/*
	创建矩形行人面域
	传入矩形对角线的两个点，startPoint为左上角，endPoint为右下角
	@param startPoint 左上角点坐标
	@param endPoint 右下角点坐标
	@return 返回创建的IPedestrianRectRegion接口指针
	*/
	virtual IPedestrianRectRegion* createPedestrianRectRegion(QPointF startPoint, QPointF endPoint);

	/*
	删除矩形行人面域
	@param pIPedestrianRectRegion 要删除的矩形面域接口指针
	*/
	virtual void removePedestrianRectRegion(IPedestrianRectRegion* pIPedestrianRectRegion);

	/*
	创建椭圆行人面域
	传入椭圆外接矩形对角线的两个点，startPoint为左上角，endPoint为右下角
	@param startPoint 左上角点坐标
	@param endPoint 右下角点坐标
	@return 返回创建的IPedestrianEllipseRegion接口指针
	*/
	virtual IPedestrianEllipseRegion* createPedestrianEllipseRegion(QPointF startPoint, QPointF endPoint);

	/*
	删除椭圆行人面域
	@param pIPedestrianEllipseRegion 要删除的椭圆面域接口指针
	*/
	virtual void removePedestrianEllipseRegion(IPedestrianEllipseRegion* pIPedestrianEllipseRegion);

	/*
	创建三角形行人面域
	三角形为矩形内接等腰三角形，传入矩形对角线的两个点，startPoint为左上角，endPoint为右下角
	@param startPoint 左上角点坐标
	@param endPoint 右下角点坐标
	@return 返回创建的IPedestrianTriangleRegion接口指针
	*/
	virtual IPedestrianTriangleRegion* createPedestrianTriangleRegion(QPointF startPoint, QPointF endPoint);

	/*
	删除三角形行人面域
	@param pIPedestrianTriangleRegion 要删除的三角形面域接口指针
	*/
	virtual void removePedestrianTriangleRegion(IPedestrianTriangleRegion* pIPedestrianTriangleRegion);

	/*
	创建扇形行人面域
	传入扇形顶点，startPoint圆心，endPoint外半径终点
	@param startPoint 圆心坐标
	@param endPoint 外半径终点坐标
	@return 返回创建的IPedestrianFanShapeRegion接口指针
	*/
	virtual IPedestrianFanShapeRegion* createPedestrianFanShapeRegion(QPointF startPoint, QPointF endPoint);

	/*
	删除扇形行人面域
	@param pIPedestrianFanShapeRegion 要删除的扇形面域接口指针
	*/
	virtual void removePedestrianFanShapeRegion(IPedestrianFanShapeRegion* pIPedestrianFanShapeRegion);

	/*
	创建多边形行人面域
	@param polygon 多边形顶点
	@return 返回创建的IPedestrianPolygonRegion接口指针
	*/
	virtual IPedestrianPolygonRegion* createPedestrianPolygonRegion(QPolygonF polygon);

	/*
	删除多边形行人面域
	@param pIPedestrianPolygonRegion 要删除的多边形面域接口指针
	*/
	virtual void removePedestrianPolygonRegion(IPedestrianPolygonRegion* pIPedestrianPolygonRegion);

	/*
	创建人行道
	@param vertexs 顶点列表
	@return 返回创建的IPedestrianSideWalkRegion接口指针
	*/
	virtual IPedestrianSideWalkRegion* createPedestrianSideWalkRegion(QList<QPointF> vertexs);

	/*
	删除人行道
	@param pIPedestrianSideWalkRegion 要删除的人行道接口指针
	*/
	virtual void removePedestrianSideWalkRegion(IPedestrianSideWalkRegion* pIPedestrianSideWalkRegion);

	/*
	创建人行横道
	@param startPoint 起点坐标
	@param endPoint 终点坐标
	@return 返回创建的IPedestrianCrossWalkRegion接口指针
	*/
	virtual IPedestrianCrossWalkRegion* createPedestrianCrossWalkRegion(QPointF startPoint, QPointF endPoint);

	/*
	删除人行横道
	@param pIPedestrianCrossWalkRegion 要删除的人行横道接口指针
	*/
	virtual void removePedestrianCrossWalkRegion(IPedestrianCrossWalkRegion* pIPedestrianCrossWalkRegion);

	/*
	创建楼梯
	@param startPoint 起点坐标
	@param endPoint 终点坐标
	@return 返回创建的IPedestrianStairRegion接口指针
	*/
	virtual IPedestrianStairRegion* createPedestrianStairRegion(QPointF startPoint, QPointF endPoint);

	/*
	删除楼梯
	@param pIPedestrianStairRegion 要删除的楼梯接口指针
	*/
	virtual void removePedestrianStairRegion(IPedestrianStairRegion* pIPedestrianStairRegion);

	/*
	创建行人发生点
	@param scenePos 场景坐标，必须在现有的面域内，否则创建失败返回nullptr
	@return 返回创建的IPedestrianPathPoint接口指针
	*/
	virtual IPedestrianPathPoint* createPedestrianPathStartPoint(QPointF scenePos);

	/*
	删除行人发生点
	@param pIPedestrianPathStartPoint 要删除的行人发生点接口指针
	*/
	virtual void removePedestrianPathStartPoint(IPedestrianPathPoint* pIPedestrianPathStartPoint);

	/*
	创建行人结束点
	@param scenePos 场景坐标，必须在现有的面域内，否则创建失败返回nullptr
	@return 返回创建的IPedestrianPathPoint接口指针
	*/
	virtual IPedestrianPathPoint* createPedestrianPathEndPoint(QPointF scenePos);

	/*
	删除行人结束点
	@param pIPedestrianPathEndPoint 要删除的行人结束点接口指针
	*/
	virtual void removePedestrianPathEndPoint(IPedestrianPathPoint* pIPedestrianPathEndPoint);

	/*
	创建行人决策点
	@param scenePos 场景坐标，必须在现有的面域内，否则创建失败返回nullptr
	@return 返回创建的IPedestrianPathPoint接口指针
	*/
	virtual IPedestrianPathPoint* createPedestrianDecisionPoint(QPointF scenePos);

	/*
	删除行人决策点
	@param pIPedestrianDecisionPoint 要删除的行人决策点接口指针
	*/
	virtual void removePedestrianDecisionPoint(IPedestrianPathPoint* pIPedestrianDecisionPoint);

	/*
	创建行人路径（或行人局部路径）
	@param pStartPoint 行人发生点（或行人决策点）
	@param pEndPoint 行人结束点
	@param middlePoints 一组中间必经点，必须在现有的面域内且从起点到中间点再到终点必须依次面域内连通，否则创建失败返回nullptr
	@return 返回创建的IPedestrianPath接口指针
	*/
	virtual IPedestrianPath* createPedestrianPath(IPedestrianPathPoint* pStartPoint, IPedestrianPathPoint* pEndPoint, QList<QPointF> middlePoints);

	/*
	删除行人路径
	@param pIPedestrianPath 要删除的行人路径接口指针
	*/
	virtual void removePedestrianPath(IPedestrianPath* pIPedestrianPath);

	/*
	创建人行横道信号灯
	@param pTrafficLight 信号机接口指针
	@param name 名称
	@param crosswalkId 人行横道ID
	@param scenePos 位于人行横道内的场景坐标
	@param isPositive 信号灯管控方向是否为正向，否则为反向
	@return 返回创建的ICrosswalkSignalLamp接口指针，如果该方向已添加过信号灯则返回nullptr
	*/
	virtual ICrosswalkSignalLamp* createCrossWalkSignalLamp(ISignalController* pTrafficLight, QString name, long crosswalkId, QPointF scenePos, bool isPositive);

	/*
	删除人行横道信号灯
	@param pICrosswalkSignalLamp 要删除的人行横道信号灯接口指针
	*/
	virtual void removeCrossWalkSignalLamp(ICrosswalkSignalLamp* pICrosswalkSignalLamp);

	///所有冲突区参数查询设置接口

	/*
	获取所有冲突点的冲突设置参数
	@return 返回冲突设置参数列表
	*/
	virtual QList<Online::Intersection::ConnectorAreaIntersectionParam> getIntersectionRelationshipParam();

	/*
	设置所有冲突点的冲突设置参数
	@param relationshipParam 冲突设置参数哈希表
	@return 设置成功返回true，否则返回false
	*/
	virtual bool setIntersectionRelationshipParam(QHash<QString, Online::Intersection::IntersectionParam> relationshipParam);

	/*
	给定冲突点ID计算初始的优先权类型
	crosspointId格式不正确返回-1
	正常返回值：路权优先级, 0-3
	0: 根据组合后车道连接ID，ID较小的具有优先权
	1: 根据组合后的车道连接ID, ID较大的具有优先权
	2: 忽略优先权，直接通行
	3: 同等路权，按照既有的算法判断谁先通行
	@param crosspointId 冲突点ID
	@return 返回优先权类型
	*/
	virtual int getInitialPriorityType(QString crosspointId);

	/*
	给定连接段ID获取所在的面域ID
	@param connectorId 连接段ID
	@return 返回面域ID，未找到返回-1
	*/
	virtual long getAreaIdByConnectorId(long connectorId);

	//////////////// 收费站相关接口

	/*
	获取收费车道列表
	@return 返回所有ITollLane接口的列表
	*/
	virtual QList<ITollLane*> tollLanes();

	/*
	获取收费决策点列表
	@return 返回所有ITollDecisionPoint接口的列表
	*/
	virtual QList<ITollDecisionPoint*> tollDecisionPoints();

	/*
	获取停车区域列表
	@return 返回所有IParkingRegion接口的列表
	*/
	virtual QList<IParkingRegion*> parkingRegions();

	/*
	获取停车决策点列表
	@return 返回所有IParkingDecisionPoint接口的列表
	*/
	virtual QList<IParkingDecisionPoint*> parkingDecisionPoints();

	/*
	根据ID查找收费车道
	@param id 收费车道ID
	@return 返回ITollLane接口指针，未找到返回nullptr
	*/
	virtual ITollLane* findTollLane(long id);

	/*
	根据ID查找收费决策点
	@param id 收费决策点ID
	@return 返回ITollDecisionPoint接口指针，未找到返回nullptr
	*/
	virtual ITollDecisionPoint* findTollDecisionPoint(long id);

	/*
	根据ID查找停车区域
	@param id 停车区域ID
	@return 返回IParkingRegion接口指针，未找到返回nullptr
	*/
	virtual IParkingRegion* findParkingRegion(long id);

	/*
	根据ID查找停车决策点
	@param id 停车决策点ID
	@return 返回IParkingDecisionPoint接口指针，未找到返回nullptr
	*/
	virtual IParkingDecisionPoint* findParkingDecisionPoint(long id);

	/*
	根据ID查找收费路径
	@param id 收费路径ID
	@return 返回ITollRouting接口指针，未找到返回nullptr
	*/
	virtual ITollRouting* findTollRouing(long id);

	/*
	根据ID查找停车路径
	@param id 停车路径ID
	@return 返回IParkingRouting接口指针，未找到返回nullptr
	*/
	virtual IParkingRouting* findParkingRouing(long id);

	/*
	移除收费车道
	@param pITollLane 要移除的收费车道接口指针
	*/
	virtual void removeTollLane(ITollLane* pITollLane);

	/*
	移除收费决策点
	@param pITollDecisionPoint 要移除的收费决策点接口指针
	*/
	virtual void removeTollDecisionPoint(ITollDecisionPoint* pITollDecisionPoint);

	/*
	移除停车区域
	@param pIParkingRegion 要移除的停车区域接口指针
	*/
	virtual void removeParkingRegion(IParkingRegion* pIParkingRegion);

	/*
	移除停车决策点
	@param pIParkingDecisionPoint 要移除的停车决策点接口指针
	*/
	virtual void removeParkingDecisionPoint(IParkingDecisionPoint* pIParkingDecisionPoint);

	/*
	移除收费路径
	@param pITollRouting 要移除的收费路径接口指针
	*/
	virtual void removeTollRouting(ITollRouting* pITollRouting);

	/*
	移除停车路径
	@param pIParkingRouting 要移除的停车路径接口指针
	*/
	virtual void removeParkingRouting(IParkingRouting* pIParkingRouting);

	/*
	根据ID移除收费车道
	@param id 收费车道ID
	*/
	virtual void removeTollLaneById(long id);

	/*
	根据ID移除收费决策点
	@param id 收费决策点ID
	*/
	virtual void removeTollDecisionPointById(long id);

	/*
	根据ID移除停车区域
	@param id 停车区域ID
	*/
	virtual void removeParkingRegionById(long id);

	/*
	根据ID移除停车决策点
	@param id 停车决策点ID
	*/
	virtual void removeParkingDecisionPointById(long id);

	/*
	根据ID移除收费路径
	@param id 收费路径ID
	*/
	virtual void removeTollRoutingById(long id);

	/*
	根据ID移除停车路径
	@param id 停车路径ID
	*/
	virtual void removeParkingRoutingById(long id);

	/*
	创建收费车道
	@param param 收费车道参数
	@return 返回创建的ITollLane接口指针
	*/
	virtual ITollLane* createTollLane(const Online::TollStation::DynaTollLane& param);

	/*
	创建停车区域
	@param param 停车区域参数
	@return 返回创建的IParkingRegion接口指针
	*/
	virtual IParkingRegion* createParkingRegion(const Online::ParkingLot::DynaParkingRegion& param);

	/*
	更新收费车道
	@param param 更新后的收费车道参数
	@return 返回更新后的ITollLane接口指针
	*/
	virtual ITollLane* updateTollLane(const Online::TollStation::DynaTollLane& param);

	/*
	更新停车区域
	@param param 更新后的停车区域参数
	@return 返回更新后的IParkingRegion接口指针
	*/
	virtual IParkingRegion* updateParkingRegion(const Online::ParkingLot::DynaParkingRegion& param);

	/*
	创建收费决策点
	@param pLink 路段接口指针
	@param distance 距路段起点距离
	@param name 决策点名称，默认为空
	@return 返回创建的ITollDecisionPoint接口指针
	*/
	virtual ITollDecisionPoint* createTollDecisionPoint(ILink* pLink, qreal distance, QString name = QString());

	/*
	创建收费路径
	@param pDeciPoint 收费决策点接口指针
	@param pITollLane 收费车道接口指针
	@return 返回创建的ITollRouting接口指针
	*/
	virtual ITollRouting* createTollRouting(ITollDecisionPoint* pDeciPoint, ITollLane* pITollLane);

	/*
	创建停车决策点
	@param pLink 路段接口指针
	@param distance 距路段起点距离
	@param name 决策点名称，默认为空
	@return 返回创建的IParkingDecisionPoint接口指针
	*/
	virtual IParkingDecisionPoint* createParkingDecisionPoint(ILink* pLink, qreal distance, QString name = QString());

	/*
	创建停车路径
	@param pDeciPoint 停车决策点接口指针
	@param pIParkingRegion 停车区域接口指针
	@return 返回创建的IParkingRouting接口指针
	*/
	virtual IParkingRouting* createParkingRouting(IParkingDecisionPoint* pDeciPoint, IParkingRegion* pIParkingRegion);

	/*
	获取收费停车时间分布列表
	@return 返回收费停车时间分布列表
	*/
	virtual QList<Online::TollStation::DynaTollParkingTimeDis> tollParkingTimeDis();

	/*
	创建收费停车时间分布
	@param param 收费停车时间分布参数
	@return 返回创建的收费停车时间分布
	*/
	virtual Online::TollStation::DynaTollParkingTimeDis createTollParkingTimeDis(const Online::TollStation::DynaTollParkingTimeDis& param);

	/*
	移除收费停车时间分布
	@param id 收费停车时间分布ID
	*/
	virtual void removeTollParkingTimeDis(long id);

	/*
	更新收费停车时间分布
	@param param 更新后的收费停车时间分布参数
	@return 返回更新后的收费停车时间分布
	*/
	virtual Online::TollStation::DynaTollParkingTimeDis updateTollParkingTimeDis(const Online::TollStation::DynaTollParkingTimeDis& param);

	/*
	获取所有停车事件分布信息
	@return 返回停车事件分布信息列表
	*/
	virtual QList<Online::ParkingLot::DynaParkingTimeDis> parkingTimeDis();

	/*
	创建停车时间分布信息
	@param param 停车时间分布参数
	@return 返回创建的停车时间分布信息
	*/
	virtual Online::ParkingLot::DynaParkingTimeDis createParkingTimeDis(const Online::ParkingLot::DynaParkingTimeDis& param);

	/*
	移除停车时间分布信息
	@param id 停车时间分布ID
	*/
	virtual void removeParkingTimeDis(long id);

	/*
	更新停车时间分布信息
	@param param 更新后的停车时间分布参数
	@return 返回更新后的停车时间分布信息
	*/
	virtual Online::ParkingLot::DynaParkingTimeDis updateParkingTimeDis(const Online::ParkingLot::DynaParkingTimeDis& param);

	/************************ 节点基础操作 ************************/

	/*
	创建节点
	@param startPoint 左上角起始点坐标
	@param endPoint 右下角终点坐标
	@param name 节点名称
	@return 返回创建的IJunction接口指针
	*/
	virtual IJunction* createJunction(QPointF startPoint, QPointF endPoint, QString name);

	/*
	查找节点
	@param junctionId 节点ID
	@return 返回IJunction接口指针，未找到返回nullptr
	*/
	virtual IJunction* findJunction(long junctionId);

	/*
	获取所有节点
	@return 返回所有IJunction接口的列表
	*/
	virtual QList<IJunction*> getAllJunctions();

	/*
	删除节点
	@param junctionId 节点ID
	@return 删除成功返回true，否则返回false
	*/
	virtual bool removeJunction(long junctionId);

	/*
	更新节点名称
	@param junctionId 节点ID
	@param name 新的节点名称
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateJunctionName(long junctionId, QString name);

	/*
	获取节点详情导出信息
	@param junctionId 节点ID
	@return 返回节点详情信息
	*/
	virtual Online::Junction::JunctionDetail getJunctionDetail(long junctionId);

	/*
	获取节点详情导出信息列表
	junctionIds为空时返回全部节点
	@param junctionIds 节点ID列表，默认为空
	@return 返回节点详情信息列表
	*/
	virtual QList<Online::Junction::JunctionDetail> getJunctionDetails(const QList<long>& junctionIds = QList<long>());

	/*
	更新节点流向信息
	@param junctionId 节点ID
	@param turnningInfos 流向信息列表
	@return 返回调用结果
	*/
	virtual Online::CallResult updateJunctionTurnInfo(long junctionId, const QList<Online::Junction::TurnningBaseInfo>& turnningInfos);

	/*
	更新节点关联信号机
	@param junctionId 节点ID
	@param signalControlId 信号机ID
	@return 返回调用结果
	*/
	virtual Online::CallResult updateJunctionSignalController(long junctionId, long signalControlId);

	/************************ 排放基础操作 ************************/

	/*
	创建评价区域
	@param startPoint 左上角起始点坐标
	@param endPoint 右下角终点坐标
	@param name 评价区域名称
	@return 返回创建的IEvaluateArea接口指针
	*/
	virtual IEvaluateArea* createEvaluateArea(QPointF startPoint, QPointF endPoint, QString name);

	/*
	查找评价区域
	@param areaId 评价区域ID
	@return 返回IEvaluateArea接口指针，未找到返回nullptr
	*/
	virtual IEvaluateArea* findEvaluateArea(long areaId);

	/*
	获取所有评价区域
	@return 返回所有IEvaluateArea接口的列表
	*/
	virtual QList<IEvaluateArea*> getAllEvaluateAreas();

	/*
	删除评价区域
	@param areaId 评价区域ID
	@return 删除成功返回true，否则返回false
	*/
	virtual bool removeEvaluateArea(long areaId);

	/*
	更新评价区域名称
	@param areaId 评价区域ID
	@param name 新的评价区域名称
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateEvaluateAreaName(long areaId, QString name);

	/*
	获取排放参数
	@return 返回排放参数JSON对象
	*/
	virtual QJsonObject getEmissionsConfig();

	/*
	设置排放参数
	@param obj 排放参数JSON对象
	@return 设置成功返回true，否则返回false
	*/
	virtual bool updateEmissionsConfig(QJsonObject obj);

	/*
	获取排放
	@param emType 排放类型
	@return 返回排放数据哈希表
	*/
	virtual QHash<QString, QHash<long, QVector<QPointF>>> getEmissions(QString emType);

	/*
	获取排放按车道分布
	@param emType 排放类型
	@return 返回按车道分布的排放数据哈希表
	*/
	virtual QHash<QString, QHash<QString, QVector<QPointF>>> getEmissionsByLane(QString emType);

	/*
	获取排放热力图数据
	@param simuTime 仿真时间
	@return 返回排放热力图数据JSON对象
	*/
	virtual QJsonObject getEmissionsHeatMapData(long simuTime);

	/************************ 时间段管理 ************************/

	/*
	获取所有时间段
	@return 返回时间段列表
	*/
	virtual QList<Online::Junction::FlowTimeInterval> getFlowTimeIntervals();

	/*
	添加时间段
	@return 返回新时间段信息，失败返回默认值
	*/
	virtual Online::Junction::FlowTimeInterval addFlowTimeInterval();

	/*
	删除时间段
	@param timeId 时间段ID
	@return 删除成功返回true，否则返回false
	*/
	virtual bool deleteFlowTimeInterval(long timeId);

	/*
	更新时间段
	@param timeId 时间段ID
	@param startTime 开始时间（秒）
	@param endTime 结束时间（秒）
	@return 返回更新后的时间段信息
	*/
	virtual Online::Junction::FlowTimeInterval updateFlowTimeInterval(long timeId, long startTime, long endTime);

	/************************ 流量管理 ************************/

	/*
	获取节点流向信息
	@param junctionId 节点ID
	@return 返回节点流向信息列表
	*/
	virtual QList<Online::Junction::FlowTurning> getJunctionFlows(long junctionId);

	/*
	更新流向流量
	@param timeId 时间段ID
	@param junctionId 节点ID
	@param turningId 转向ID
	@param inputFlowValue 输入流量值（辆/小时）
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateFlow(long timeId, long junctionId, long turningId, long inputFlowValue);

	/************************ 算法参数设置 ************************/

	/*
	更新流量算法参数
	@param theta 参数θ（0.01-1）
	@param bpra BPR路阻参数A（0.05-0.5）
	@param bprb BPR路阻参数B（1-10）
	@param maxIterateNum 最大迭代次数（1-5000）
	@return 更新成功返回true，否则返回false
	*/
	virtual bool updateFlowAlgorithmParams(double theta, double bpra, double bprb, long maxIterateNum);

	/************************ 路径构建 ************************/

	/*
	构建并应用路径
	@param minPathNum 最小路径数，默认为3
	@return 返回路径结果映射：<起始路段ID,终点路段ID> -> 可行路径列表
	*/
	virtual QMap<QPair<long, long>, QVector<QList<ILink*>>> buildAndApplyPaths(long minPathNum = 3);

	/*
	优化决策点位置
	*/
	virtual void reSetDeciPoint();

	/*
	优化连接段
	@param pITollRouting 决策路径接口指针
	*/
	virtual void reSetLaneConnector(IRouting* pITollRouting);

	/************************ 流量计算 ************************/

	/*
	计算并应用流量结果
	@return 返回时间段ID到流量计算结果的映射
	*/
	virtual QHash<long, QList<Online::Junction::FlowTurning>> calculateFlows();

	/**************用于录制视频****************/

	/*
	录制视频
	@param path 视频保存路径
	@return 返回录制状态信息
	*/
	virtual QString Recordvideo(QString path);

	/*
	停止录屏
	@return 返回停止状态信息
	*/
	virtual QString stopScreenRecording();

};

#endif // NETINTERFACE_H
