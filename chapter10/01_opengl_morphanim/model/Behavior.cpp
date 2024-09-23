#include "Behavior.h"

#include "Logger.h"

/* use internal logging as callback by default  */
Behavior::Behavior() {
  mInstanceNodeActionCallback = [this](int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    updateInstanceSettings(instanceId, nodeType, updateType, data, extraSetting);
  };
}

void Behavior::updateInstanceSettings(int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  Logger::log(1, "%s: got a callback for instance %i, setting speed to %f, extra setting to %s\n", __FUNCTION__,
              instanceId, std::get<float>(data), extraSetting ? "true" : "false");
}

void Behavior::setNodeActionCallback(instanceNodeActionCallback callbackFunction) {
  mInstanceNodeActionCallback = callbackFunction;
}

void Behavior::update(float deltaTime) {
  for (auto& instance : mInstanceToBehaviorMap) {
    instance.second.update(deltaTime);
  }
}

void Behavior::clear() {
  mInstanceToBehaviorMap.clear();
}

void Behavior::addInstance(int instanceId, std::shared_ptr<SingleInstanceBehavior> behavior) {
  /* remove before insert */
  removeInstance(instanceId);

  /* copy data from pointer */
  mInstanceToBehaviorMap.emplace(std::make_pair(instanceId, SingleInstanceBehavior(*behavior)));

  mInstanceToBehaviorMap[instanceId].setInstanceId(instanceId);
  mInstanceToBehaviorMap[instanceId].setInstanceNodeActionCallback(mInstanceNodeActionCallback);

  std::shared_ptr<BehaviorData> behaviorData = mInstanceToBehaviorMap[instanceId].getBehaviorData();
  Logger::log(1, "%s: added behavior for instance %i with %i nodes and %i links (%i total behaviors)\n",
    __FUNCTION__, mInstanceToBehaviorMap[instanceId].getInstanceId() , behaviorData->bdGraphNodes.size(),
    behaviorData->bdGraphLinks.size(), mInstanceToBehaviorMap.size());
}

void Behavior::removeInstance(int instanceId) {
  if (mInstanceToBehaviorMap.count(instanceId) < 0) {
    Logger::log(1, "%s warning: no behavior for instance %i was set\n", __FUNCTION__, instanceId);
    return;
  }

  std::shared_ptr<BehaviorData> behaviorData = mInstanceToBehaviorMap[instanceId].getBehaviorData();
  std::string removedBehaviorName = behaviorData->bdName.c_str();

  mInstanceToBehaviorMap[instanceId].deactivateAll();
  mInstanceToBehaviorMap.erase(instanceId);

  Logger::log(1, "%s: removed behavior %s from instance %i\n", __FUNCTION__, behaviorData->bdName.c_str(), instanceId);
}

void Behavior::addEvent(int instanceId, nodeEvent event) {
  if (mInstanceToBehaviorMap.count(instanceId) == 0) {
    Logger::log(1, "%s error: node id %i not found in behavior map\n", __FUNCTION__, instanceId);
    return;
  }
  mInstanceToBehaviorMap[instanceId].addEvent(event);
}
