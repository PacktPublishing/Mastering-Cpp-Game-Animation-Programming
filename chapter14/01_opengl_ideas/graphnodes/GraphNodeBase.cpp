#include "GraphNodeBase.h"

#include "Logger.h"

GraphNodeBase::GraphNodeBase(int nodeId) : mNodeId(nodeId) {}

void GraphNodeBase::setNodeOutputTriggerCallback(fireNodeOutputCallback callbackFunction) {
  mNodeCallbackFunction = callbackFunction;
}

void GraphNodeBase::fireNodeOutputTriggerCallback(int outId) {
  if (mNodeCallbackFunction) {
    mNodeCallbackFunction(outId);
  } else {
    Logger::log(1, "%s error: callback not bound\n", __FUNCTION__);
  }
}

void GraphNodeBase::setNodeActionCallback(nodeActionCallback callbackFunction) {
  mNodeActionCallbackFunction = callbackFunction;
}

void GraphNodeBase::fireNodeActionCallback(graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  if (mNodeActionCallbackFunction) {
    mNodeActionCallbackFunction(nodeType, updateType, data, extraSetting);
  } else {
    Logger::log(1, "%s error: callback not bound\n", __FUNCTION__);
  }
}
