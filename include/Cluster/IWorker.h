#ifndef __IWorker__
#define __IWorker__

#include "tessinterfaces_global.h"

#include <QList>
#include <QSet>

#include <memory>
#include <unordered_set>

class IVehicle;
class ISection;

struct Snapshot;
namespace TessSnapshot {
	class _Snapshot;
}

class TESSINTERFACES_EXPORT IWorker
{
public:
	virtual ~IWorker() = default;
	//初始化
	virtual void init() = 0;
	//获取 Worker 标识符
	virtual int workerKey() = 0;
	//驶入某TESSNG节占
	virtual void travelToOtherTessNode(IVehicle* pVehicle) = 0;
	//起动仿真
	virtual void startSimuWithSnapshot(TessSnapshot::_Snapshot* pValue) = 0;
	//分配计算节点路段，由tessng调用
	virtual bool distributeLink(QList<QList<long>>& llLinkId, long step = 0) = 0;
	//捕获快照
	virtual std::shared_ptr<TessSnapshot::_Snapshot> catchSnapshot() = 0;
	//捕获快照
	virtual std::string catchSnapshotAsString() = 0;
	//抓怕部分快照
	virtual std::shared_ptr<TessSnapshot::_Snapshot> catchPartialSnapshot(const QSet<long> &linkIdSet, const QSet<long> &connIdSet) = 0;
	//发送快照
	virtual void sendSnapshot(TessSnapshot::_Snapshot *pSnapshot) = 0;
	//计算下一点后的处理
	virtual void afterStep(IVehicle* pIVehicle) = 0;
	//第step批次计算完成后的处理
	virtual void afterOneStep(long step = 0) = 0;
	//停止工作时
	virtual void afterStop() = 0;
	//获取最近一次快照
	virtual TessSnapshot::_Snapshot* takeSnapshot() = 0;
	//接收快照输入
	virtual void recvSnapshot(const TessSnapshot::_Snapshot& pSnapshot) = 0;

	//开启平行仿真
	virtual bool syncStartParallelOf(const std::string& key) = 0;
	//通过快照开启平行仿真
	virtual bool syncStartParallelOf(const std::string& key, const std::string& bytes) = 0;
	//开启一组平行仿真
	virtual int syncStartParallelOf(const std::unordered_set<std::string>& keys) = 0;
	//开启所有的平行仿真
	virtual int syncStartAllParallel() = 0;
};


#endif