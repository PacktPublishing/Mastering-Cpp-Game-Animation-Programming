#pragma once

#include <memory>
#include <set>

#include "BehaviorData.h"
#include "Callbacks.h"
#include "Enums.h"

class SingleInstanceBehavior {
  public:
    SingleInstanceBehavior();
    explicit SingleInstanceBehavior(const SingleInstanceBehavior& orig);

    void update(float deltaTime, bool tiggerRoot = true);
    void updateNodeStatus(int pinId);

    void deactivateAll(bool informParentNodes = true);

    void setBehaviorData(std::shared_ptr<BehaviorData> data);
    std::shared_ptr<BehaviorData> getBehaviorData() const;

    void setInstanceId(int instanceId);
    int getInstanceId() const;

    void setInstanceNodeActionCallback(instanceNodeActionCallback callbackFunction);
    void debugInstanceNodeCallback(int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);
    void nodeActionCallback(graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);

    void addEvent(nodeEvent event);

  private:
    fireNodeOutputCallback mFireNodeOutputCallbackFunction;
    instanceNodeActionCallback mInstanceNodeActionCallbackFunction;

    std::shared_ptr<BehaviorData> mBehaviorData = nullptr;
    int mInstanceId = 0;

    std::vector<nodeEvent> mPendingNodeEvents{};
    std::vector<nodeEvent> mNewPendingNodeEvents{};
};
