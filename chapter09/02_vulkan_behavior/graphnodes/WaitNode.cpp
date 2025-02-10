#include "WaitNode.h"

#include "Logger.h"

WaitNode::WaitNode(int nodeId, float waitTime) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;

  mWaitTime = waitTime;
  mCurrentTime = mWaitTime;
}

std::shared_ptr<GraphNodeBase> WaitNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new WaitNode(*this));
}

void WaitNode::draw(ModelInstanceCamData modInstCamData) {
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  if (mActive) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(0, 255, 0, 255)));
  }
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  if (mActive) {
    ImGui::PopStyleColor();
  }
  ImNodes::EndNodeTitleBar();

  ImNodes::BeginInputAttribute(mInId);
  ImGui::Text("in");
  ImNodes::EndInputAttribute();

  if (mActive) {
    ImGui::BeginDisabled();
  }

  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##Float", &mWaitTime, 0.0f, 25.0f, "%.3fs", flags);

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    mCurrentTime = mWaitTime;
  }

  ImGui::Text("Left: %4.2fs", mCurrentTime);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  if (mActive) {
    ImGui::EndDisabled();
  }

  ImNodes::BeginOutputAttribute(mOutId);
  if (mFired) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(0, 255, 0, 255)));
  }
  ImGui::Text("          out");
  if (mFired) {
    ImGui::PopStyleColor();
  }
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void WaitNode::activate() {
  if (mActive) {
    Logger::log(2, "%s warning: node %i already active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mActive = true;
  mFired = false;
}

void WaitNode::deactivate(bool informParentNodes) {
  if (!mActive) {
    Logger::log(2, "%s warning: node %i not active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mActive = false;
  mFired = false;
  mCurrentTime = mWaitTime;

  if (informParentNodes) {
    /* inform parent that we are done */
    fireNodeOutputTriggerCallback(mInId);
  }
}

void WaitNode::update(float deltaTime) {
  if (!mActive) {
    return;
  }

  mCurrentTime -= deltaTime;

  if (mCurrentTime <= 0.0f) {
    /* notify child(s) */
    fireNodeOutputTriggerCallback(mOutId);
    /* notify parent(s) */
    fireNodeOutputTriggerCallback(mInId);

    mCurrentTime = mWaitTime;
    mActive = false;
    mFired = true;
  }
}

std::optional<std::map<std::string, std::string>> WaitNode::exportData() {
  std::map<std::string, std::string> data{};
  data["wait-time"] = std::to_string(mWaitTime);
  return data;
}

void WaitNode::importData(std::map<std::string, std::string> data) {
  mWaitTime = std::stof(data["wait-time"]);
  mCurrentTime = mWaitTime;
}
