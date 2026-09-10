#pragma once
#include <qobject.h>

#include <unordered_map>

#include "tessinterfaces_global.h"

//#include "google/protobuf/message.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

class TESSINTERFACES_EXPORT ITessClient :
    public QObject
{
public:
    ITessClient();
    virtual ~ITessClient();

    virtual bool isActive();

    virtual uint64_t getCurSyncBatchNum();
    virtual uint64_t getWorkerBatchNum(long node_id);
    virtual uint64_t getWorkerNewestBatchNum(long node_id);
    // node_id: msg
    virtual std::unordered_map<int, google::protobuf::Message*> getNextFullBatchMsgs(bool bAllowResetCurBatch = false) = 0;
    // 合并所给的数据
    virtual bool mergeMsgs(const std::unordered_map<int, google::protobuf::Message*>& hMsgs, google::protobuf::Message* mergedMsg, bool bMore = true) = 0;
    // 获得合并后的批次数据
    virtual bool getNextFullBatchMsgsMerged(google::protobuf::Message* mergedMsg, bool bAllowResetCurBatch = false, bool bMore = true) = 0;
    virtual uint64_t getNewestFullBatchNum();

    virtual bool isInited();
    virtual bool isStarted();
    virtual bool isStarting();
};
