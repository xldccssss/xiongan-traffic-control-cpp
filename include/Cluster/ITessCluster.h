#ifndef __ITessCluster__
#define __ITessCluster__

#include "tessinterfaces_global.h"
#include "IDistributedManager.h"

class IWorker;
class IMaster;
class IParallelSimulator;
class NetInterface;
class SimuInterface;

class TESSINTERFACES_EXPORT ITessCluster
{
public:
	virtual ~ITessCluster() = default;
	virtual void init() = 0;
	virtual IWorker* worker() = 0;
	virtual IMaster* master() = 0;
	virtual bool hasDistributed() = 0;
	virtual std::shared_ptr<IDistributedManager> getDistributedManager() = 0;
	virtual IParallelSimulator* parallelSimulator() = 0;
	//=====================

	virtual NetInterface* netInterface() = 0;
	virtual SimuInterface* simuInterface() = 0;
};

extern ITessCluster TESSINTERFACES_EXPORT *gpTessCluster;

#endif