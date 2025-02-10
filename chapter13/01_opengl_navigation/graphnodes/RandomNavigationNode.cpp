#include "RandomNavigationNode.h"

#include "Logger.h"

RandomNavigationNode::RandomNavigationNode(int nodeId) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;
}

std::shared_ptr<GraphNodeBase> RandomNavigationNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new RandomNavigationNode(*this));
}

void RandomNavigationNode::draw(ModelInstanceCamData modInstCamData) {
  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  ImNodes::BeginInputAttribute(mInId);
  ImGui::Text("in");
  ImNodes::EndInputAttribute();

  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Use Random Nav Target");
  ImNodes::EndStaticAttribute();

  ImNodes::BeginOutputAttribute(mOutId);
  ImGui::Text("                  out");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void RandomNavigationNode::activate() {
  instanceUpdateType updateType = instanceUpdateType::navigation;
  nodeCallbackVariant result; /* ignored */
  bool extra = false;
  fireNodeActionCallback(getNodeType(), updateType, result, extra);

  Logger::log(2, "%s: node '%s' activated navigation \n", __FUNCTION__, getNodeName().c_str());

  /* pass through control flow */
  fireNodeOutputTriggerCallback(mOutId);
}
