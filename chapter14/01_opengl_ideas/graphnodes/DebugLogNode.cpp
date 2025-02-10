#include "DebugLogNode.h"

#include "Logger.h"

DebugLogNode::DebugLogNode(int nodeId) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;
}

std::shared_ptr<GraphNodeBase> DebugLogNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new DebugLogNode(*this));
}

void DebugLogNode::draw(ModelInstanceCamData modInstCamData) {
  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  /* In pin */
  ImNodes::BeginInputAttribute(mInId);
  ImGui::Text("in");
  ImNodes::EndInputAttribute();

  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Debug");
  if (mActive) {
    ImGui::Text("(Active)");
  } else {
    ImGui::BeginDisabled();
    ImGui::Text("(Inactive)");
    ImGui::EndDisabled();
  }
  ImNodes::EndStaticAttribute();

  /* out pin */
  ImNodes::BeginOutputAttribute(mOutId);
  ImGui::Text("       out");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void DebugLogNode::activate() {
  mActive = true;
  fireNodeOutputTriggerCallback(mOutId);
  Logger::log(1, "%s: == debug node %i triggered ==\n", __FUNCTION__, getNodeId());
}

void DebugLogNode::deactivate(bool informParentNodes) {
  mActive = false;
}
