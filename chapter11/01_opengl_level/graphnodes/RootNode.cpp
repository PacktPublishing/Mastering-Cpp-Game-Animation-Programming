#include "RootNode.h"

RootNode::RootNode() : GraphNodeBase(0) {
  mOutId = 0;
}


std::shared_ptr<GraphNodeBase> RootNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new RootNode(*this));
}

void RootNode::draw(ModelInstanceCamData modInstCamData) {
  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  ImNodes::BeginOutputAttribute(mOutId);
  ImGui::Text("        out");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void RootNode::activate() {
  fireNodeOutputTriggerCallback(mOutId);
}


