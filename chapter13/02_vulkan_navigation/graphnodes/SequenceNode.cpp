#include "SequenceNode.h"

#include "Logger.h"

SequenceNode::SequenceNode(int nodeId, int numOut) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = getNodeId() * 1000;
  mInId = id;
  mOutIdStart = id + 200;

  for (int i = 0; i < numOut; ++i) {
    mOutIds.emplace_back(i);
  }
}

std::shared_ptr<GraphNodeBase> SequenceNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new SequenceNode(*this));
}

void SequenceNode::draw(ModelInstanceCamData modInstCamData) {
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

void SequenceNode::activate() {
  if (mActive) {
    Logger::log(1, "%s warning: node %i already active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mActive = true;

  mActiveOut = 0;
  fireNodeOutputTriggerCallback(mOutIds.at(mActiveOut) + mOutIdStart);
}

void SequenceNode::deactivate(bool informParentNodes) {
  if (!mActive) {
    Logger::log(1, "%s warning: node %i not active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mActive = false;
  mActiveOut = -1;
}

void SequenceNode::childFinishedExecution() {
  if (!mActive) {
    Logger::log(1, "%s warning: node %i not active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mActiveOut++;
  if (mActiveOut == mOutIds.size()) {
    mActive = false;
    mActiveOut = -1;
    return;
  }

  Logger::log(2, "%s: activate out %i (%i) of node %i\n", __FUNCTION__, mActiveOut, mOutIds.at(mActiveOut) + mOutIdStart, getNodeId());
  fireNodeOutputTriggerCallback(mOutIds.at(mActiveOut) + mOutIdStart);
}

void SequenceNode::addOutputPin() {
  mOutIds.emplace_back(mOutIds.size());
}

int SequenceNode::delOutputPin() {
  if (mOutIds.size() > 2) {
    mOutIds.pop_back();
  }
  return mOutIds.size() + mOutIdStart;
}

int SequenceNode::getNumOutputPins() {
  return mOutIds.size();
}

std::optional<std::map<std::string, std::string>> SequenceNode::exportData() {
  std::map<std::string, std::string> data{};
  data["sequence-num-outs"] = std::to_string(mOutIds.size());
  return data;
}

void SequenceNode::importData(std::map<std::string, std::string> data) {
  int goalNumOuts = std::stoi(data["sequence-num-outs"]);
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
}
