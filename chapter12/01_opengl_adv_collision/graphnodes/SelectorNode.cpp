#include "SelectorNode.h"

#include "Logger.h"

SelectorNode::SelectorNode::SelectorNode(int nodeId, float waitTime, int numOut) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = getNodeId() * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutIdStart = id + 200;

  for (int i = 0; i < numOut; ++i) {
    mOutIds.emplace_back(i);
  }

  mWaitTime = waitTime;
  mCurrentTime = mWaitTime;
}

std::shared_ptr<GraphNodeBase> SelectorNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new SelectorNode(*this));
}

void SelectorNode::draw(ModelInstanceCamData modInstCamData) {
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

  for (int i = 0; i < mOutIds.size(); ++i) {
    ImNodes::BeginOutputAttribute(mOutIds.at(i) + mOutIdStart);
    if (i == mActiveOut) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(0, 255, 0, 255)));
    }
    ImGui::Text("        out %2i", i + 1);
    if (i == mActiveOut) {
      ImGui::PopStyleColor();
    }
    ImNodes::EndOutputAttribute();
  }

  ImNodes::EndNode();
}

void SelectorNode::activate() {
  if (mActive) {
    Logger::log(2, "%s warning: node %i already active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mActive = true;
}

void SelectorNode::deactivate(bool informParentNodes) {
  if (!mActive) {
    Logger::log(2, "%s warning: node %i not active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mCurrentTime = mWaitTime;
  mActiveOut = -1;
  mActive = false;
}

void SelectorNode::update(float deltaTime) {
  if (!mActive) {
    return;
  }

  mCurrentTime -= deltaTime;

  if (mCurrentTime <= 0.0f) {
    mActiveOut = std::rand() % mOutIds.size();
    Logger::log(2, "%s: activate out %i (%i) of node %i\n", __FUNCTION__, mActiveOut, mOutIds.at(mActiveOut) + mOutIdStart, getNodeId());

    /* fire random output */
    fireNodeOutputTriggerCallback(mOutIds.at(mActiveOut) + mOutIdStart);
    /* inform parent that we triggered */
    fireNodeOutputTriggerCallback(mInId);

    mCurrentTime = mWaitTime;
    mActive = false;
  }
}

void SelectorNode::addOutputPin() {
  mOutIds.emplace_back(mOutIds.size());
}

int SelectorNode::delOutputPin() {
  if (mOutIds.size() > 2) {
    mOutIds.pop_back();
  }
  return mOutIds.size() + mOutIdStart;
}

int SelectorNode::getNumOutputPins() {
  return mOutIds.size();
}

std::optional<std::map<std::string, std::string>> SelectorNode::exportData() {
  std::map<std::string, std::string> data{};
  data["selector-wait-time"] = std::to_string(mWaitTime);
  data["selector-num-outs"] = std::to_string(mOutIds.size());
  return data;
}

void SelectorNode::importData(std::map<std::string, std::string> data) {
  int goalNumOuts = std::stoi(data["selector-num-outs"]);
  int numOuts = mOutIds.size();
  if (goalNumOuts > numOuts) {
    for (int i = 0; i < goalNumOuts - numOuts; ++i) {
      addOutputPin();
    }
  } else if (goalNumOuts < numOuts) {
    for (int i = 0; i < numOuts - goalNumOuts; ++i) {
      delOutputPin();
    }
  }
  mWaitTime = std::stof(data["selector-wait-time"]);
  mCurrentTime = mWaitTime;
}
