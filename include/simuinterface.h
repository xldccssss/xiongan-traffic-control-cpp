/********************************************************************************
* @file simuinterface.h
* @brief TESS NG仿真运算层接口
* @details 此接口由TESS NG实现，用户可以通过此接口：
*          - 启动/停止/暂停仿真
*          - 获取所有车辆运行状态
*          - 获取指定车辆运行轨迹
*          - 获取检测器数据（采集器、排队计数器、行程时间检测器等）
*          - 动态创建车辆、公交车
*          - 获取信号灯状态
*          - 管理仿真参数（精度、加速倍数、随机数种子等）
*********************************************************************************/

#ifndef SIMUINTERFACE_H
#define SIMUINTERFACE_H

#include <QObject>
#include <QPolygonF>

#include "tessinterfaces_global.h"
#include "Plugin/_datastruct.h"
#include "UnitChange.h"

class CustomerSimulator;
class IVehicleSupervisor;
class IVehicleDrivingManagerTask;
class IVehicleDrivingTask;
class IVehicle;
class IBusLine;
class IPedestrian;
class SimulatorBatchCommands;

class TESSINTERFACES_EXPORT SimuInterface
{
public:
	// 全局下一批次控制代理，由仿真接口持有，不保存命令帧状态。
	class Step
	{
	public:
		TESSINTERFACES_EXPORT void setSimuConfig(const Online::SimuConfig& config);
		TESSINTERFACES_EXPORT void setDispatchParameters(const QList<Online::DispatchInterval>& parameters);
		TESSINTERFACES_EXPORT void setFlowRatioParameters(const QList<Online::DecipointFlowRatioByInterval>& parameters);
		TESSINTERFACES_EXPORT void setSignalControlParameters(const QList<Online::SignalContralParam>& parameters);
		TESSINTERFACES_EXPORT void setFollowingParams(const QList<Online::FollowingModelParam>& parameters);
		TESSINTERFACES_EXPORT void setFollowingParamsByVehicleType(const QList<Online::FollowingParamByVehiType>& parameters);
		TESSINTERFACES_EXPORT void setIfCalcDistToReduceArea(bool ifCalc);
		TESSINTERFACES_EXPORT void setRecentSpeedAndPosTime(int seconds);

	private:
		Step() = default;

		friend class SimuInterface;
	};

	SimuInterface();
	virtual ~SimuInterface();

	/*
	获取车辆驾驶管理任务
	@return 返回车辆驾驶管理任务接口指针
	*/
	virtual IVehicleDrivingManagerTask* vehicleDrivingManagerTask();

	/*
	获取车辆驾驶任务列表
	@return 返回所有车辆驾驶任务的列表
	*/
	virtual QList<IVehicleDrivingTask*> vehicleDrivingTasks();

	/*
	判断仿真时间是否由CPU时间确定
	@return 如果使用CPU时间确定仿真时间返回true，否则返回false
	*/
	virtual bool byCpuTime();

	/*
	设置是否由现实时间(处理器运算时间)确定仿真时间
	@param bByCpuTime true-使用CPU时间，false-使用仿真时间
	@return 如果正在仿真计算则不进行设置并返回false，否则设置参数并返回true
	*/
	virtual bool setByCpuTime(bool bByCpuTime);

	/*
	启动仿真
	*/
	virtual void startSimu();

	/*
	单步仿真（执行一个仿真步长）
	*/
	virtual void stepSimu();

	/*
	暂停仿真
	*/
	virtual void pauseSimu();

	/*
	结束仿真
	*/
	virtual void stopSimu();

	/*
	判断仿真是否在进行中
	@return 如果仿真正在运行返回true，否则返回false
	*/
	virtual bool isRunning();

	/*
	判断仿真是否处于暂停状态
	@return 如果仿真处于暂停状态返回true，否则返回false
	*/
	virtual bool isPausing();

	/*
	判断是否记录车辆轨迹
	@return 如果记录车辆轨迹返回true，否则返回false
	*/
	virtual bool isRecordTrace();

	/*
	设置是否记录车辆轨迹
	@param bRecord true-记录车辆轨迹，false-不记录
	*/
	virtual void setIsRecordTrace(bool bRecord);

	/*
	判断是否记录行人轨迹
	@return 如果记录行人轨迹返回true，否则返回false
	*/
	virtual bool isRecordPedestrianTrace();

	/*
	设置是否记录行人轨迹
	@param bRecord true-记录行人轨迹，false-不记录
	*/
	virtual void setIsRecordPedestrianTrace(bool bRecord);

	/*
	判断是否记录信号灯灯态
	@return 如果记录信号灯灯态返回true，否则返回false
	*/
	virtual bool isRecordSignalLampStatus();

	/*
	设置是否记录信号灯灯态
	@param bRecord true-记录信号灯灯态，false-不记录
	*/
	virtual void setIsRecordSignalLampStatus(bool bRecord);

	/*
	获取预期仿真时长
	@return 返回预期仿真时长（毫秒）
	*/
	virtual long simuIntervalScheming();

	/*
	设置预期仿真时长
	@param interval 仿真时长（毫秒）
	*/
	virtual void setSimuIntervalScheming(long interval);

	/*
	获取仿真精度
	@return 返回仿真精度（每批次代表的毫秒数）
	*/
	virtual int simuAccuracy();

	/*
	设置仿真精度
	@param accuracy 仿真精度（每批次代表的毫秒数）
	*/
	virtual void setSimuAccuracy(int accuracy);

	/*
	获取加速倍数
	@return 返回加速倍数
	*/
	virtual int acceMultiples();

	/*
	设置加速倍数
	@param multiples 加速倍数
	*/
	virtual void setAcceMultiples(int multiples);

	/*
	设置工作线程数
	@param count 线程数
	*/
	virtual void setThreadCount(int count);

	/*
	获取随机数种子
	@return 返回随机数种子
	*/
	virtual int randSeed();

	/*
	设置随机数种子
	@param seed 随机数种子
	*/
	virtual void setRandSeed(int seed);

	/*
	获取实际加速倍数（受系统性能影响）
	@return 返回实际加速倍数
	*/
	virtual qreal acceMultiplesReally();

	/*
	获取当前仿真批次
	@return 返回当前批次号
	*/
	virtual long batchNumber();

	/*
	获取指定批次的实时时间戳
	@param batchNumber 批次号
	@return 返回时间戳（毫秒）
	*/
	virtual qint64 timeOnBatchNumber(long batchNumber);

	/*
	获取当前批次实际耗时
	@return 返回耗时（毫秒）
	*/
	virtual qreal batchIntervalReally();

	/*
	获取仿真开始的现实时间戳
	@return 返回时间戳（毫秒）
	*/
	virtual qint64 startMSecsSinceEpoch();

	/*
	获取仿真结束的现实时间戳
	@return 返回时间戳（毫秒）
	*/
	virtual qint64 stopMSecsSinceEpoch();

	/*
	获取当前已仿真时间（考虑加速倍数）
	@return 返回已仿真时间（毫秒）
	*/
	virtual long simuTimeIntervalWithAcceMutiples();

	//仿真到指定批次时总延误(毫秒)
	//virtual long delayTimeOnBatchNumber(long batchNumber);

	/*
	获取车辆总数（已生成车辆数）
	@return 返回车辆总数
	*/
	virtual long vehiCountTotal();

	/*
	获取路网上正在运行的车辆数
	@return 返回正在运行的车辆数
	*/
	virtual long vehiCountRunning();

	/*
	根据车辆ID获取车辆对象
	@param vehiId 车辆ID
	@return 返回IVehicle接口指针
	*/
	virtual IVehicle* getVehicle(long vehiId);

	/*
	获取所有正在运行的车辆
	@return 返回所有运行中车辆的列表
	*/
	virtual QList<IVehicle*> allVehiStarted();

	/*
	获取所有车辆（包括已完成仿真的）
	@return 返回所有车辆的列表
	*/
	virtual QList<IVehicle*> allVehicle();

	/*
	获取所有在运行车辆状态（包括位置、速度等）
	@param batchNumber 指定批次，0表示当前批次
	@param unit 计量单位
	@return 返回车辆状态列表
	*/
	virtual QList<Online::VehicleStatus> getVehisStatus(long batchNumber = 0, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取指定车辆的运行轨迹
	@param vehiId 车辆ID
	@param unit 计量单位
	@return 返回车辆位置列表
	*/
	virtual QList<Online::VehiclePosition> getVehiTrace(long vehiId, UnitOfMeasure unit = UnitOfMeasure::Default);

	/*
	获取当前所有信号灯组的相位颜色
	@param bPlanInCurrSimuTime true-仅返回当前仿真时段有效的信控方案
	@return 返回信号灯相位颜色列表
	*/
	virtual QList<Online::SignalPhaseColor> getSignalPhasesColor(bool bPlanInCurrSimuTime = false);

	/*
	获取当前仿真时间完成穿越采集器的所有车辆信息
	@return 返回采集到的车辆信息列表
	*/
	virtual QList<Online::VehiInfoCollected> getVehisInfoCollected();

	/*
	获取最近集计时间段内采集器采集的所有车辆集计信息
	@return 返回集计信息列表
	*/
	virtual QList<Online::VehiInfoAggregated> getVehisInfoAggregated();

	/*
	获取当前仿真时间排队计数器计数的车辆排队信息
	@return 返回排队信息列表
	*/
	virtual QList<Online::VehiQueueCounted> getVehisQueueCounted();

	/*
	获取最近集计时间段内排队计数器集计数据
	@return 返回集计数据列表
	*/
	virtual QList<Online::VehiQueueAggregated> getVehisQueueAggregated();

	/*
	获取当前仿真时间行程时间检测器完成的行程时间检测信息
	@return 返回行程时间检测信息列表
	*/
	virtual QList<Online::VehiTravelDetected> getVehisTravelDetected();

	/*
	获取最近集计时间段内行程时间检测器集计数据
	@return 返回集计数据列表
	*/
	virtual QList<Online::VehiTravelAggregated> getVehisTravelAggregated();

	/*
	动态创建车辆
	@param dynaVehi 车辆参数
	@param bStarted 是否立即启动
	@param bInitLaneObject 是否初始化车道或车道连接，false则从车道或车道连接起点发车
	@return 返回创建的车辆接口指针
	*/
	virtual IVehicle* createGVehicle(Online::DynaVehiParam dynaVehi, bool bStarted = true, bool bInitLaneObject = true);

	/*
	动态创建公交车
	@param pBusLine 公交线路
	@param startSimuDateTime 发车仿真时间
	@return 返回创建的公交车接口指针
	*/
	virtual IVehicle* createBus(IBusLine* pBusLine, qreal startSimuDateTime);

	/*
	创建仿真快照（需要TESSNG分布式组件支持）
	@return 返回快照字符串
	*/
	virtual QString catchSnapshotAsString();

	/*
	从快照字符串加载仿真状态（需要TESSNG分布式组件支持）
	@param data 快照字符串
	@return 加载成功返回true，否则返回false
	*/
	virtual bool loadSnapshotFromString(QString data);

	/*
	停止指定车辆的运行
	@param pVehicle 车辆接口指针
	*/
	virtual void stopVehicleDriving(IVehicle* pVehicle);

	/*
	暂停或恢复仿真（切换状态）
	*/
	virtual void pauseSimuOrNot();

	/*
	获取指定路段上的所有车辆
	@param linkId 路段ID
	@return 返回路段上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInLink(long linkId);

	/*
	获取指定车道上的所有车辆
	@param laneId 车道ID
	@return 返回车道上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInLane(long laneId);

	/*
	获取指定连接段上的所有车辆
	@param connectorId 连接段ID
	@return 返回连接段上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInConnector(long connectorId);

	/*
	获取指定车道连接器上的所有车辆
	@param connectorId 连接段ID
	@param fromLaneId 上游车道ID
	@param toLaneId 下游车道ID
	@return 返回车道连接器上的车辆列表
	*/
	virtual QList<IVehicle*> vehisInLaneConnector(long connectorId, long fromLaneId, long toLaneId);

	/*
	计算路段（Section）的交通状态
	@return 返回路段交通状态列表
	*/
	virtual QList<Online::SectionStatus> calcTrafficStatus();

	/*
	获取排队计数器最近一次排队信息
	@param queueCounterId 排队计数器ID
	@param queueLength 输出参数，排队长度
	@param vehiCount 输出参数，排队车辆数
	@param unit 计量单位，默认为默认单位（像素）
	@return 是否获取成功
	*/
	virtual bool queueRecently(long queueCounterId, qreal& queueLength, int& vehiCount, UnitOfMeasure unit = UnitOfMeasure::Default);

	//行人相关接口

	/*
	根据行人面域ID获取当前时间面域上所有行人的状态信息
	@param regionId 行人面域ID
	@return 返回行人状态列表
	*/
	virtual QList<Online::Pedestrian::PedestrianStatus> getPedestriansStatusByRegionId(long regionId);

	/*
	获取所有正在运行的行人
	@return 返回正在运行的行人列表
	*/
	virtual QList<IPedestrian*> allPedestrianStarted();

	/*
	获取已经生成的行人总数
	@return 返回已生成的行人总数
	*/
	virtual int getGeneratedPedestrianCount();

	/*
	获取当前正在运行的行人数量
	@return 返回正在运行的行人数量
	*/
	virtual int	getActivePedestrianCount();

	/*
	获取下一批次控制代理。返回指针由当前仿真接口持有，不能跨越接口生命周期保留。
	*/
	Step* step();

private:
	static SimulatorBatchCommands* currentStepCommands(const char* methodName);
	Step* mStep = nullptr;
};

#endif // SIMUINTERFACE_H
