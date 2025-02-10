#include "TestNode.h"

TestNode::TestNode(int nodeId) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mStaticIdStart = id + 100;
  mOutId = id + 200;
}


std::shared_ptr<GraphNodeBase> TestNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new TestNode(*this));
}

void TestNode::draw(ModelInstanceCamData modInstCamData) {
  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  if (ImGui::Button("  Test  ")) {
    fireOutput();
  }
  ImNodes::EndStaticAttribute();

  ImNodes::BeginOutputAttribute(mOutId);
  ImGui::Text("        out");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void TestNode::fireOutput() {
  fireNodeOutputTriggerCallback(mOutId);
}
