#include "SingleInstanceBehavior.h"

#include <algorithm>

#include "GraphNodeBase.h"
#include "Logger.h"

SingleInstanceBehavior::SingleInstanceBehavior() {
  mBehaviorData = std::make_shared<BehaviorData>();
  mFireNodeOutputCallbackFunction = [this](int pinId) { updateNodeStatus(pinId); };

  mInstanceNodeActionCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance,
      graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    debugInstanceNodeCallback(instance, nodeType, updateType, data, extraSetting);
  };
  mBehaviorData->bdNodeActionCallbackFunction = [this](graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    nodeActionCallback(nodeType, updateType, data, extraSetting);
  };
}

/* custom copy constructor, making a copy of the given nodes and links in Behavior */
SingleInstanceBehavior::SingleInstanceBehavior(const SingleInstanceBehavior& orig) {
  mFireNodeOutputCallbackFunction = [this](int pinId) { updateNodeStatus(pinId); };

  mBehaviorData = std::make_shared<BehaviorData>();
  mBehaviorData->bdNodeActionCallbackFunction = [this](graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    nodeActionCallback(nodeType, updateType, data, extraSetting);
  };

  std::shared_ptr<BehaviorData> origBehavior = orig.getBehaviorData();
  mBehaviorData->bdGraphLinks = origBehavior->bdGraphLinks;
  mBehaviorData->bdName = origBehavior->bdName;

  for (const auto& node : origBehavior->bdGraphNodes) {
    std::shared_ptr<GraphNodeBase> newNode = node->clone();
    if (node->getNodeType() == graphNodeType::instanceMovement || node->getNodeType() == graphNodeType::action ||
        node->getNodeType() == graphNodeType::faceAnim || node->getNodeType() == graphNodeType::headAmin ||
        node->getNodeType() == graphNodeType::randomNavigation) {
      newNode->setNodeActionCallback(mBehaviorData->bdNodeActionCallbackFunction);
    }
    newNode->setNodeOutputTriggerCallback(mFireNodeOutputCallbackFunction);
    mBehaviorData->bdGraphNodes.emplace_back(std::move(newNode));
  }

  mInstanceNodeActionCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance,
      graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    debugInstanceNodeCallback(instance, nodeType, updateType, data, extraSetting);
  };
}

void SingleInstanceBehavior::update(float deltaTime, bool triggerRoot) {
  /* no data, no updates */
  if (mBehaviorData->bdGraphNodes.size() == 1) {
    return;
  }

  /* normal path update */
  for (const auto& node: mBehaviorData->bdGraphNodes) {
    node->update(deltaTime);;
  }

  for (auto iter = mPendingNodeEvents.begin(); iter != mPendingNodeEvents.end(); /* forwarded in loop */ ) {
    bool eventHandled = false;
    for (const auto& node: mBehaviorData->bdGraphNodes) {
      if (node->getNodeType() == graphNodeType::event && node->listensToEvent(*iter)) {
        node->handleEvent();
        eventHandled = true;
      }
    }

    if (eventHandled) {
         iter = mPendingNodeEvents.erase(iter);
      } else {
        ++iter;
    }
  }

  /* append new pending events */
  mPendingNodeEvents.insert(mPendingNodeEvents.end(), mNewPendingNodeEvents.begin(), mNewPendingNodeEvents.end());
  mNewPendingNodeEvents.clear();

  /* check if we have any active nodes */
  unsigned int activeNodes = 0;
  for (const auto& node: mBehaviorData->bdGraphNodes) {
    if (node->isActive()) {
      activeNodes++;
    }
  }

  /* (re)-trigger root node if we don't have any active nodes left  */
  if (triggerRoot && activeNodes == 0) {
    //Logger::log(1, "%s: no active nodes left, trigger root \n" , __FUNCTION__);
    mBehaviorData->bdGraphNodes.at(0)->activate();
  }
}

void SingleInstanceBehavior::deactivateAll(bool informParentNodes) {
  for (const auto& node: mBehaviorData->bdGraphNodes) {
    node->deactivate(informParentNodes);
  }
}

std::shared_ptr<BehaviorData> SingleInstanceBehavior::getBehaviorData() const {
  return mBehaviorData;
}

void SingleInstanceBehavior::setBehaviorData(std::shared_ptr<BehaviorData> data) {
  mBehaviorData = data;
}

void SingleInstanceBehavior::setInstance(std::shared_ptr<AssimpInstance> instance) {
  mInstance = instance;
}

std::shared_ptr<AssimpInstance> SingleInstanceBehavior::getInstance() const {
  std::shared_ptr<AssimpInstance> instance = mInstance.lock();
  if (!instance) {
    return nullptr;
  }
  return instance;
}

void SingleInstanceBehavior::addEvent(nodeEvent event) {
  mNewPendingNodeEvents.emplace_back(event);
}

void SingleInstanceBehavior::debugInstanceNodeCallback(std::shared_ptr<AssimpInstance> instance,
    graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  Logger::log(1, "%s: got update from instance %i (node type %i, update type %i) (%x)\n", __FUNCTION__,
    instance->getInstanceIndexPosition(), nodeType, updateType, this);
}

/* transform callback to include instance id instead of node type */
void SingleInstanceBehavior::nodeActionCallback(graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  std::shared_ptr<AssimpInstance> instance = mInstance.lock();
  if (!instance) {
    Logger::log(1, "%s error: instance not found\n", __FUNCTION__);
    return;
  }

  if (mInstanceNodeActionCallbackFunction && instance) {
    mInstanceNodeActionCallbackFunction(instance, nodeType, updateType, data, extraSetting);
  } else {
    Logger::log(1, "%s error: callback not bound on instance %i\n", __FUNCTION__,
      instance->getInstanceIndexPosition());
  }
}

void SingleInstanceBehavior::setInstanceNodeActionCallback(instanceNodeActionCallback callbackFunction) {
  mInstanceNodeActionCallbackFunction = callbackFunction;
}

void SingleInstanceBehavior::updateNodeStatus(int pinId) {
  int nodeId = pinId / 1000;
  Logger::log(2, "%s: triggered from pin %i of node %i (%x)\n", __FUNCTION__, pinId, nodeId, this);

  /* current output is always the 1st element of the pair */
  /* current input is always the 2nd element of the pair */

  /* search parent node by storing output to the input from node just triggered */
  std::vector<int> parentIds{};
  for (const auto& link : mBehaviorData->bdGraphLinks) {
    if (link.second.second == pinId) {
      parentIds.emplace_back(link.second.first);
    }
  }

  if (!parentIds.empty()) {
    for (const auto& parentNodePin : parentIds) {
      const int destNodeId = parentNodePin / 1000;
      Logger::log(2, "%s: found output %i on node %i\n", __FUNCTION__, parentNodePin, destNodeId);
      const auto node = std::find_if(mBehaviorData->bdGraphNodes.begin(), mBehaviorData->bdGraphNodes.end(),
                                     [destNodeId](std::shared_ptr<GraphNodeBase> node) { return node->getNodeId() == destNodeId; } );
      if (node != mBehaviorData->bdGraphNodes.end()) {
        Logger::log(2, "%s: inform parent node %i\n", __FUNCTION__, destNodeId);
        (*node)->childFinishedExecution();
      } else {
        Logger::log(1, "%s error: no output %i of node %i no longer connected?!\n", __FUNCTION__, parentNodePin, destNodeId);
      }
    }
    /* we have either input or output pins - return if we found parent node(s) */
    return;
  }


  /* search child nodes */
  std::vector<int> childIds{};
  for (const auto& link : mBehaviorData->bdGraphLinks) {
    if (link.second.first == pinId) {
      childIds.emplace_back(link.second.second);
    }
  }

  /* HACK: if no child node was found: tell parent node that execution finished */
  if (childIds.empty()) {
    Logger::log(2, "%s warning: no other node connected to input %i of node %i\n", __FUNCTION__, pinId, nodeId);
    const auto node = std::find_if(mBehaviorData->bdGraphNodes.begin(), mBehaviorData->bdGraphNodes.end(),
                                   [nodeId](std::shared_ptr<GraphNodeBase> node) { return node->getNodeId() == nodeId; } );
    if (node != mBehaviorData->bdGraphNodes.end()) {
      Logger::log(2, "%s: unconnected pin, inform parent node %i\n", __FUNCTION__, nodeId);
      (*node)->childFinishedExecution();
    }
    return;
  }

  for (const auto& childNodePin : childIds) {
    const int destNodeId = childNodePin / 1000;
    Logger::log(2, "%s: found input %i on node %i\n", __FUNCTION__, childNodePin, destNodeId);
    const auto node = std::find_if(mBehaviorData->bdGraphNodes.begin(), mBehaviorData->bdGraphNodes.end(),
                                   [destNodeId](std::shared_ptr<GraphNodeBase> node) { return node->getNodeId() == destNodeId; } );
    if (node != mBehaviorData->bdGraphNodes.end()) {
      Logger::log(2, "%s: activate node %i\n", __FUNCTION__, destNodeId);
      (*node)->activate();
    } else {
      Logger::log(2, "%s warning: input %i of node %i not connected\n", __FUNCTION__, childNodePin, destNodeId);
    }
  }
}
