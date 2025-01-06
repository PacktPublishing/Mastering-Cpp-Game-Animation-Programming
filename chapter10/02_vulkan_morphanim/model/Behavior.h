#pragma once

#include <memory>
#include <unordered_map>

#include "SingleInstanceBehavior.h"
#include "ModelInstanceCamData.h"
#include "AssimpInstance.h"
#include "Callbacks.h"
#include "Enums.h"

class Behavior {
  public:
    Behavior();

    void addInstance(int instanceId, std::shared_ptr<SingleInstanceBehavior> behavior);
    void removeInstance(int instanceId);
    void update(float deltaTime);
    void addEvent(int instanceId, nodeEvent event);
    void clear();

    void setNodeActionCallback(instanceNodeActionCallback callbackFunction);

  private:
    void updateInstanceSettings(int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);

    std::map<int, SingleInstanceBehavior> mInstanceToBehaviorMap{};

    instanceNodeActionCallback mInstanceNodeActionCallback;
};
