#pragma once

#include <memory>
#include <unordered_map>

#include "SingleInstanceBehavior.h"
#include "ModelInstanceCamData.h"
#include "AssimpInstance.h"
#include "Callbacks.h"
#include "Enums.h"

class BehaviorManager {
  public:
    BehaviorManager();

    void addInstance(std::shared_ptr<AssimpInstance> instance, std::shared_ptr<SingleInstanceBehavior> behavior);
    void removeInstance(std::shared_ptr<AssimpInstance> instance);
    void update(float deltaTime);
    void addEvent(std::shared_ptr<AssimpInstance> instance, nodeEvent event);
    void clear();

    void setNodeActionCallback(instanceNodeActionCallback callbackFunction);

  private:
    void updateInstanceSettings(std::shared_ptr<AssimpInstance> instance, graphNodeType nodeType,
      instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);

    std::map<std::weak_ptr<AssimpInstance>, SingleInstanceBehavior,
    std::owner_less<std::weak_ptr<AssimpInstance>>> mInstanceToBehaviorMap{};

    instanceNodeActionCallback mInstanceNodeActionCallbackFunction;
};
