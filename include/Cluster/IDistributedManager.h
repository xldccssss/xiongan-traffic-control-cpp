//
// Created by jd on 2023/2/27.
//

#ifndef TESSNG_IDISTRIBUTEDMANAGER_H
#define TESSNG_IDISTRIBUTEDMANAGER_H

#include "Plugin/OnlineInterface.h"

#include "IConjDsWithTess.h"

class TESSINTERFACES_EXPORT IDistributedManager {
protected:
    std::atomic_bool mbStarted{ false };
public:
    enum class Status : char {
        Invalid = 0,
        Master = 1,
        Worker = 2
    };
    virtual ~IDistributedManager() = default;
    virtual void init();
    virtual void quickInit();
    virtual void start() = 0;
    virtual void stop();
    virtual void stopSimu();
    virtual bool isStarted() {
        return mbStarted;
    }
    virtual std::shared_ptr<IConjDsWithTess> getConj();
    virtual void* getJobExecutor();
    virtual bool isMaster();
    virtual void setOnlineInterface(std::shared_ptr<Online::OnlineInterface> onlineInterface);
    virtual const QJsonObject& getDistributedJsonConfig();
    virtual bool isReadyForOnline();
    virtual bool isWorking();
};


#endif //TESSNG_IDISTRIBUTEDMANAGER_H
