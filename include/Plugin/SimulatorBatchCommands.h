#ifndef SIMULATORBATCHCOMMANDS_H
#define SIMULATORBATCHCOMMANDS_H

#include <QHash>
#include <QJsonValue>
#include <QList>
#include <QMutex>
#include <QString>
#include <QVector3D>

#include "../tessinterfaces_global.h"
#include "_datastruct.h"

class CustomerSimulator;
class GVehicle;
class ILane;
class ISignalLamp;
class IVehicle;
class SimuInterface;
class TESS;
class VehicleDriving;
class VehicleDrivingManagerTask;

// 内部车辆命令有效位，用于区分未设置字段和显式零值。
enum class VehicleBatchCommandField : quint64
{
	DesirSpeed = 1ULL << 0,
	Speed = 1ULL << 1,
	Acce = 1ULL << 2,
	FollowingType = 1ULL << 3,
	FollowingParam = 1ULL << 4,
	FrontVehicle = 1ULL << 5,
	FrontDistance = 1ULL << 6,
	LeftLaneChange = 1ULL << 7,
	RightLaneChange = 1ULL << 8,
	LeftLaneChangeFreely = 1ULL << 9,
	RightLaneChangeFreely = 1ULL << 10,
	DismissLaneChange = 1ULL << 11,
	IfCalcNextRoad = 1ULL << 12,
	CandidateLaneConnectors = 1ULL << 13,
	NextLaneConnector = 1ULL << 14,
	IfCalcNextPoint = 1ULL << 15,
	LimitedLanes = 1ULL << 16,
	MaxLimitedSpeed = 1ULL << 17,
	DistanceToEvent = 1ULL << 18,
	ChangeLaneSafeDistance = 1ULL << 19,
	VehicleEuler = 1ULL << 20,
	Angle = 1ULL << 21
};

enum class GlobalBatchCommandField : quint64
{
	SimuConfig = 1ULL << 0,
	DispatchParameters = 1ULL << 1,
	FlowRatioParameters = 1ULL << 2,
	SignalControlParameters = 1ULL << 3,
	FollowingParams = 1ULL << 4,
	FollowingParamsByVehicleType = 1ULL << 5,
	IfCalcDistToReduceArea = 1ULL << 6,
	RecentSpeedAndPosTime = 1ULL << 7
};

struct TESSINTERFACES_EXPORT VehicleBatchCommand
{
	long vehicleId = 0;
	// 内部有效位用于区分“未设置”和显式设置为零值。
	quint64 validMask = 0;
	qreal desirSpeed = 0;
	qreal speed = 0;
	qreal acce = 0;
	int followingType = 0;
	qreal safeInterval = 0;
	qreal safeDistance = 0;
	long frontVehicleId = 0;
	qreal frontDistance = 0;
	qreal stopDistance = 0;
	bool leftLaneChange = false;
	bool rightLaneChange = false;
	bool leftLaneChangeFreely = false;
	bool rightLaneChangeFreely = false;
	bool dismissLaneChange = false;
	bool ifCalcNextRoad = false;
	bool ifCalcNextPoint = false;
	QList<long> candidateLaneConnectorIds;
	long nextLaneConnectorId = 0;
	QList<int> limitedLaneNumbers;
	qreal maxLimitedSpeed = 0;
	qreal distanceToEvent = 0;
	qreal changeLaneSafeDistance = 0;
	QVector3D vehicleEuler;
	qreal angle = 0;
};

struct TESSINTERFACES_EXPORT LinkLaneBatchCommand
{
	long linkId = 0;
	int laneNumber = 0;
	qreal speedLimitKph = 0;
};

struct TESSINTERFACES_EXPORT SignalLampBatchCommand
{
	long signalLampId = 0;
	QString color;
};

struct TESSINTERFACES_EXPORT GlobalBatchCommand
{
	Online::SimuConfig simuConfig;
	QList<Online::DispatchInterval> dispatchParameters;
	QList<Online::DecipointFlowRatioByInterval> flowRatioParameters;
	QList<Online::SignalContralParam> signalControlParameters;
	QList<Online::FollowingModelParam> followingParams;
	QList<Online::FollowingParamByVehiType> followingParamsByVehicleType;
	bool ifCalcDistToReduceArea = false;
	int recentSpeedAndPosTime = 0;
};

struct TESSINTERFACES_EXPORT SimulatorCommandFrame
{
	long sourceBatch = -1;
	long targetBatch = -1;
	quint64 globalValidMask = 0;
	GlobalBatchCommand globalCommand;
	QHash<long, VehicleBatchCommand> vehicleCommands;
	QHash<long, SignalLampBatchCommand> signalCommands;
	QList<LinkLaneBatchCommand> linkLaneCommands;
};

// C++ 持有的命令帧容器；任意线程只写 pendingFrame，仿真线程仅在批次边界发布只读 activeFrame。
class TESSINTERFACES_EXPORT SimulatorBatchCommands
{
public:
	void setSimuConfig(const Online::SimuConfig& config);
	void setDispatchParameters(const QList<Online::DispatchInterval>& parameters);
	void setFlowRatioParameters(const QList<Online::DecipointFlowRatioByInterval>& parameters);
	void setSignalControlParameters(const QList<Online::SignalContralParam>& parameters);
	void setFollowingParams(const QList<Online::FollowingModelParam>& parameters);
	void setFollowingParamsByVehicleType(const QList<Online::FollowingParamByVehiType>& parameters);
	void setLampColor(long signalLampId, const QString& color);
	void setIfCalcDistToReduceArea(bool ifCalc);
	void setRecentSpeedAndPosTime(int seconds);

	void setDesirSpeed(long vehicleId, qreal speed);
	void setSpeed(long vehicleId, qreal speed);
	void setAcce(long vehicleId, qreal acce);
	void setFollowingType(long vehicleId, int followingType);
	void setFollowingParam(long vehicleId, qreal safeInterval, qreal safeDistance);
	void setFrontVehicle(long vehicleId, long frontVehicleId);
	void setFrontDistance(long vehicleId, qreal frontDistance, qreal stopDistance);
	void setLeftLaneChange(long vehicleId, bool enabled);
	void setRightLaneChange(long vehicleId, bool enabled);
	void setLeftLaneChangeFreely(long vehicleId, bool enabled);
	void setRightLaneChangeFreely(long vehicleId, bool enabled);
	void setDismissLaneChange(long vehicleId, bool enabled);
	void setIfCalcNextRoad(long vehicleId, bool ifCalc);
	void setCandidateLaneConnectors(long vehicleId, const QList<long>& laneConnectorIds);
	void setNextLaneConnector(long vehicleId, long laneConnectorId);
	void setIfCalcNextPoint(long vehicleId, bool ifCalc);
	void setLimitedLanes(long vehicleId, const QList<int>& laneNumbers);
	// 车道限速单位固定为 km/h。
	void setLaneSpeedLimit(long linkId, int laneNumber, qreal speedLimitKph);
	void setMaxLimitedSpeed(long vehicleId, qreal speed);
	void setDistanceToEvent(long vehicleId, qreal distance);
	void setChangeLaneSafeDistance(long vehicleId, qreal distance);
	void setVehicleEuler(long vehicleId, const QVector3D& euler);
	void setAngle(long vehicleId, qreal angle);

private:
	SimulatorBatchCommands() = default;
	SimulatorBatchCommands(const SimulatorBatchCommands&) = delete;
	SimulatorBatchCommands& operator=(const SimulatorBatchCommands&) = delete;

	// 返回当前插件持有的命令容器，对象级接口可在其生命周期内随时写入下一帧。
	static SimulatorBatchCommands* currentCommands(const char* methodName);
	void bindAsCurrent();
	void unbindAsCurrent();
	VehicleBatchCommand* pendingVehicleCommand(long vehicleId, VehicleBatchCommandField field);
	void beginCollect(long sourceBatch, long targetBatch);
	void commit();
	void activate();
	void reset();
	const SimulatorCommandFrame& activeFrame() const;
	const VehicleBatchCommand* vehicleCommand(long vehicleId) const;
	const SignalLampBatchCommand* signalCommand(long signalLampId) const;
	bool laneSpeedLimit(long linkId, int laneNumber, qreal& speedLimitKph) const;
	// 验证环境开启时，仅记录已经进入真实消费路径的指令。
	void recordVehicleFieldConsumed(long vehicleId, VehicleBatchCommandField field) const;
	void recordGlobalFieldConsumed(GlobalBatchCommandField field) const;
	void recordConsumedCommand(const QString& methodName, const QString& objectId, const QJsonValue& actual) const;

	QMutex mPendingMutex;
	mutable QMutex mConsumptionMutex;
	SimulatorCommandFrame mPendingFrame;
	SimulatorCommandFrame mCommittedFrame;
	SimulatorCommandFrame mActiveFrame;
	static QMutex msCurrentCommandsMutex;
	static SimulatorBatchCommands* msCurrentCommands;

	friend class CustomerSimulator;
	friend class GVehicle;
	friend class ILane;
	friend class ISignalLamp;
	friend class IVehicle;
	friend class SimuInterface;
	friend class TESS;
	friend class VehicleDriving;
	friend class VehicleDrivingManagerTask;
};

#endif // SIMULATORBATCHCOMMANDS_H
