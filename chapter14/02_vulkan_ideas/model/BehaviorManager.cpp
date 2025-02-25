#include "BehaviorManager.h"

#include "Logger.h"

/* use internal logging as callback by default  */
BehaviorManager::BehaviorManager() {
  mInstanceNodeActionCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance, graphNodeType nodeType,
      instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    updateInstanceSettings(instance, nodeType, updateType, data, extraSetting);
  };
}

void BehaviorManager::updateInstanceSettings(std::shared_ptr<AssimpInstance> instance, graphNodeType nodeType,
    instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  Logger::log(1, "%s: got a callback for instance %i, setting speed to %f, extra setting to %s\n", __FUNCTION__,
    instance->getInstanceIndexPosition(), std::get<float>(data), extraSetting ? "true" : "false");
}

void BehaviorManager::setNodeActionCallback(instanceNodeActionCallback callbackFunction) {
  mInstanceNodeActionCallbackFunction = callbackFunction;
}

void BehaviorManager::update(float deltaTime) {
  for (auto& instance : mInstanceToBehaviorMap) {
    instance.second.update(deltaTime);
  }
}

void BehaviorManager::clear() {
  mInstanceToBehaviorMap.clear();
}

void BehaviorManager::addInstance(std::shared_ptr<AssimpInstance> instance, std::shared_ptr<SingleInstanceBehavior> behavior) {
  /* remove before insert */
  removeInstance(instance);

  /* copy data from pointer */
  mInstanceToBehaviorMap.emplace(std::make_pair(instance, SingleInstanceBehavior(*behavior)));

  mInstanceToBehaviorMap[instance].setInstance(instance);
  mInstanceToBehaviorMap[instance].setInstanceNodeActionCallback(mInstanceNodeActionCallbackFunction);

  std::shared_ptr<BehaviorData> behaviorData = mInstanceToBehaviorMap[instance].getBehaviorData();
  Logger::log(1, "%s: added behavior for instance %i with %i nodes and %i links (%i total behaviors)\n",
    __FUNCTION__, mInstanceToBehaviorMap[instance].getInstance()->getInstanceIndexPosition(), behaviorData->bdGraphNodes.size(),
    behaviorData->bdGraphLinks.size(), mInstanceToBehaviorMap.size());
}

void BehaviorManager::removeInstance(std::shared_ptr<AssimpInstance> instance) {
  if (mInstanceToBehaviorMap.count(instance) == 0) {
    Logger::log(1, "%s warning: no behavior for instance %i was set\n", __FUNCTION__, instance->getInstanceIndexPosition());
    return;
  }

  std::shared_ptr<BehaviorData> behaviorData = mInstanceToBehaviorMap[instance].getBehaviorData();
  std::string removedBehaviorName = behaviorData->bdName.c_str();

  mInstanceToBehaviorMap[instance].deactivateAll();
  mInstanceToBehaviorMap.erase(instance);

  Logger::log(1, "%s: removed behavior %s from instance %i\n", __FUNCTION__, behaviorData->bdName.c_str(), instance->getInstanceIndexPosition());
}

void BehaviorManager::addEvent(std::shared_ptr<AssimpInstance> instance, nodeEvent event) {
  if (mInstanceToBehaviorMap.count(instance) == 0) {
    Logger::log(1, "%s error: node id %i not found in behavior map\n", __FUNCTION__, instance->getInstanceIndexPosition());
    return;
  }
  mInstanceToBehaviorMap[instance].addEvent(event);
}
