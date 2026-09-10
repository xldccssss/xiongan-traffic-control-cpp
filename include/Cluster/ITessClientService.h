#pragma once
#include <unordered_map>

#include "tessinterfaces_global.h"

namespace Tess {
	class TessMsg;
}

class TESSINTERFACES_EXPORT ITessClientService: public QObject
{
	Q_OBJECT
protected:
	uint64_t mStartBatch{ 0 };
	std::unordered_map<void*, int> mHandlePtr2NodeId;
	std::unordered_map<int, Tess::TessMsg*> mCurFullBatchMsgs;
public:
	virtual ~ITessClientService() = default;
	// 期望工作线程数量
	virtual int expHandleCnt();
	// 注册工作线程(指针)
	virtual bool registerHandlePtr(void* ptr);
	// 下载最新批次数据; return 最新批次号; 会阻塞
	virtual uint64_t downNextFullBatchMsgs();
	// 更新给定工作线程负责的车辆
	virtual void updateVehisByPtr(void* ptr);
	// 是否可用
	virtual bool isActive();
	// 获取给定线程的当前批次数据
	const Tess::TessMsg* getTessMsgByPtr(void* ptr) const;
	// 起始批次号
	uint64_t getStartBatchNum() {
		return mStartBatch;
	}
};

extern QSharedPointer<ITessClientService> TESSINTERFACES_EXPORT gpTessClientService;