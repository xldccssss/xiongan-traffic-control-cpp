#ifndef __IParallelSimulator__
#define __IParallelSimulator__

#include "tessinterfaces_global.h"

#include <unordered_set>

class TESSINTERFACES_EXPORT IParallelSimulator
{
public:
	virtual ~IParallelSimulator() = default;
	virtual void parallelSimu() = 0;
	virtual void setSnapshotData(QByteArray data) = 0;
	virtual bool recoveredSnapshot() = 0;
};

#endif
