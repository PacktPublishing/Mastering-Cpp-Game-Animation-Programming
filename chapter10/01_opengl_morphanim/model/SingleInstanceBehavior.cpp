#include "SingleInstanceBehavior.h"

#include <algorithm>

#include "GraphNodeBase.h"
#include "Logger.h"

SingleInstanceBehavior::SingleInstanceBehavior() {
  mBehaviorData = std::make_shared<BehaviorData>();
  mFireNodeOutputCallback = [this](int pinId) { updateNodeStatus(pinId); };

  mInstanceNodeActionCallback = [this](int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    debugInstanceNodeCallback(instanceId, nodeType, updateType, data, extraSetting);
  };
  mBehaviorData->bdNodeActionCallbackFunction = [this](graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    nodeActionCallback(nodeType, updateType, data, extraSetting);
  };
}

/* custom copy constructor, making a copy of the given nodes and links in Behavior */
SingleInstanceBehavior::SingleInstanceBehavior(const SingleInstanceBehavior& orig) {
  mFireNodeOutputCallback = [this](int pinId) { updateNodeStatus(pinId); };

  mBehaviorData = std::make_shared<BehaviorData>();
  mBehaviorData->bdNodeActionCallbackFunction = [this](graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    nodeActionCallback(nodeType, updateType, data, extraSetting);
  };

  std::shared_ptr<BehaviorData> origBehavior = orig.getBehaviorData();
  mBehaviorData->bdGraphLinks = origBehavior->bdGraphLinks;
  mBehaviorData->bdName = origBehavior->bdName;

  for (const auto& node : origBehavior->bdGraphNodes) {
    std::shared_ptr<GraphNodeBase> newNode = node->clone();
    if (node->getNodeType() == graphNodeType::instance || node->getNodeType() == graphNodeType::action ||
        node->getNodeType() == graphNodeType::faceAnim || node->getNodeType() == graphNodeType::headAmin) {
      newNode->setNodeActionCallback(mBehaviorData->bdNodeActionCallbackFunction);
    }
    newNode->setNodeOutputTriggerCallback(mFireNodeOutputCallback);
    mBehaviorData->bdGraphNodes.emplace_back(std::move(newNode));
  }

  mInstanceId = orig.mInstanceId;
  mInstanceNodeActionCallback = [this](int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    debugInstanceNodeCallback(instanceId, nodeType, updateType, data, extraSetting);
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

  /* event nodes disable all nodes if the node handles the event */
  for (const auto& node: mBehaviorData->bdGraphNodes) {
    for (auto iter = mPendingNodeEvents.begin(); iter != mPendingNodeEvents.end(); /* forwarded in loop */ ) {
      if (node->getNodeType() == graphNodeType::event && node->listensToEvent(*iter)) {
         node->handleEvent();
         iter = mPendingNodeEvents.erase(iter);
      } else {
        ++iter;
      }
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

void SingleInstanceBehavior::setInstanceId(int instanceId) {
  mInstanceId = instanceId;
}

int SingleInstanceBehavior::getInstanceId() const {
  return mInstanceId;
}

void SingleInstanceBehavior::addEvent(nodeEvent event) {
  mNewPendingNodeEvents.emplace_back(event);
}

void SingleInstanceBehavior::debugInstanceNodeCallback(int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  Logger::log(1, "%s: got update from instance %i (node type %i, update type %i) (%x)\n", __FUNCTION__, instanceId, nodeType, updateType, this);
}

/* transform callback to include instance id instead of node type */
void SingleInstanceBehavior::nodeActionCallback(graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  if (mInstanceNodeActionCallback) {
    mInstanceNodeActionCallback(mInstanceId, nodeType, updateType, data, extraSetting);
  } else {
    Logger::log(1, "%s error: callback not bound\n", __FUNCTION__);
  }
}

void SingleInstanceBehavior::setInstanceNodeActionCallback(instanceNodeActionCallback callbackFunction) {
  mInstanceNodeActionCallback = callbackFunction;
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
