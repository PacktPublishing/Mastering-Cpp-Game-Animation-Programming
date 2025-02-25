#pragma once

#include <memory>
#include <set>

#include "BehaviorData.h"
#include "Callbacks.h"
#include "Enums.h"
#include "AssimpInstance.h"

class SingleInstanceBehavior {
  public:
    SingleInstanceBehavior();
    explicit SingleInstanceBehavior(const SingleInstanceBehavior& orig);

    void update(float deltaTime, bool tiggerRoot = true);
    void updateNodeStatus(int pinId);

    void deactivateAll(bool informParentNodes = true);

    void setBehaviorData(std::shared_ptr<BehaviorData> data);
    std::shared_ptr<BehaviorData> getBehaviorData() const;

    void setInstance(std::shared_ptr<AssimpInstance> instance);
    std::shared_ptr<AssimpInstance> getInstance() const;

    void setInstanceNodeActionCallback(instanceNodeActionCallback callbackFunction);
    void debugInstanceNodeCallback(std::shared_ptr<AssimpInstance> instance, graphNodeType nodeType,
      instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);
    void nodeActionCallback(graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);

    void addEvent(nodeEvent event);

  private:
    fireNodeOutputCallback mFireNodeOutputCallbackFunction;
    instanceNodeActionCallback mInstanceNodeActionCallbackFunction;

    std::shared_ptr<BehaviorData> mBehaviorData = nullptr;
    std::weak_ptr<AssimpInstance> mInstance;

    std::vector<nodeEvent> mPendingNodeEvents{};
    std::vector<nodeEvent> mNewPendingNodeEvents{};
};
