#ifndef __IMaster__
#define __IMaster__

#include "tessinterfaces_global.h"
#include "netinterface.h"

#include <QPolygonF>
#include <QMultiMap>

//#include "Cluster/_Snapshot.h"
//#include "Cluster/Snapshot.pb.h"
namespace TessSnapshot {
	class _Snapshot;
}

struct Snapshot;
class IWorker;

struct TESSINTERFACES_EXPORT WorkerNode {
	int workerKey;
	QString ip;
	int port;
	QPolygonF polygon;
	//本区域面域ID集
	QList<long> lConnAreaId;
	//本区域路段ID集
	QList<long> lLinkId;
	//本区域连接段ID集
	QList<long> lConnectorId;
	//下游Worker节点相关边缘路段
	QMultiMap<int, long> mapToWKeyLink;
	//上游Worker节点与当前TESSNG节点路段关系
	QMultiMap<int, long> mapFromWKeyLink;

	WorkerNode()
	{
		workerKey = -1;
		port = 0;
	}
};

class TESSINTERFACES_EXPORT IMaster
{
public:
	virtual ~IMaster() = default;
	//初始化
	virtual void init() = 0;
	//增加节点
	virtual void addWorker(IWorker* pWorker) = 0;
	//根据TESSNG节点标识查找worker
	virtual IWorker* findWorker(int tessKey) = 0;
	//为计算节点分配路段
	virtual void distributeLinkForWorker() = 0;
	//为各仿真节点分配路段
	virtual bool dispatchSectionForSimunode(QList<WorkerNode*> &lWorkerNode) = 0;
	//启动计算结节的仿真计算
	virtual void startSimuWithSnapshot(IWorker* pIWorker, TessSnapshot::_Snapshot* pSnapshot) = 0;
	//启动计算结节的仿真计算
	virtual void startSimuWithSnapshot(TessSnapshot::_Snapshot* pSnapshot) = 0;
	//平行仿真
	virtual void parallelSimu() = 0;
};

#endif