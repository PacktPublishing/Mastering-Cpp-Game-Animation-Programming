#include "RandomWaitNode.h"

#include "Logger.h"

RandomWaitNode::RandomWaitNode(int nodeId, float minWaitTime, float maxWaitTime) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;

  mMinWaitTime = minWaitTime;
  mMaxWaitTime = maxWaitTime;

  calculateRandomWaitTime();
}

std::shared_ptr<GraphNodeBase> RandomWaitNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new RandomWaitNode(*this));
}

/* we need a custom copy constructor to create a random time after cloning */
RandomWaitNode::RandomWaitNode(const RandomWaitNode& orig) : GraphNodeBase(orig.getNodeId()) {
  mInId = orig.mInId;
  mOutId = orig.mOutId;
  mStaticIdStart = orig.mStaticIdStart;

  mActive = false;
  mFired = false;
  mMinWaitTime = orig.mMinWaitTime;
  mMaxWaitTime = orig.mMaxWaitTime;

  calculateRandomWaitTime();
}

void RandomWaitNode::calculateRandomWaitTime() {
  if (mMaxWaitTime < mMinWaitTime) {
    mMaxWaitTime = mMinWaitTime;
    mCurrentTime = mMinWaitTime;
  } else {
    mCurrentTime = mMinWaitTime;
    if (std::fabs(mMaxWaitTime - mMinWaitTime) > 0.001f) {
      mCurrentTime = std::rand() % static_cast<int>((mMaxWaitTime - mMinWaitTime) * 1000) + mMinWaitTime * 1000;
      if (std::fabs(mCurrentTime) > 0.001f) {
        mCurrentTime /= 1000.0f;
      }
    }
  }
}

void RandomWaitNode::draw(ModelInstanceCamData modInstCamData) {
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

  /* attributes: nodeId  * 1000, ascending */
  ImNodes::BeginInputAttribute(mInId);
  ImGui::Text("in");
  ImNodes::EndInputAttribute();

  if (mActive) {
    ImGui::BeginDisabled();
  }

  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Min:");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##FloatMin", &mMinWaitTime, 0.0f, 25.0f, "%.3fs", flags);
  ImGui::PopItemWidth();

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    calculateRandomWaitTime();
  }
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);

  ImGui::Text("Max:");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##FloatMax", &mMaxWaitTime, 0.0f, 25.0f, "%.3fs", flags);

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    calculateRandomWaitTime();
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
  ImGui::Text("              out");
  if (mFired) {
    ImGui::PopStyleColor();
  }
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void RandomWaitNode::activate() {
  if (mActive) {
    Logger::log(2, "%s warning: node %i already active, ignoring\n", __FUNCTION__, getNodeId());
    return;
  }

  mActive = true;
  mFired = false;
}

void RandomWaitNode::deactivate(bool informParentNodes) {
  if (!mActive) {
    return;
  }

  calculateRandomWaitTime();
  mActive = false;
  mFired = false;

  if (informParentNodes) {
    /* inform parent that we are done */
    fireNodeOutputTriggerCallback(mInId);
  }
}

void RandomWaitNode::update(float deltaTime) {
  if (!mActive) {
    return;
  }

  mCurrentTime -= deltaTime;

  if (mCurrentTime <= 0.0f) {
    /* notify child(s) */
    fireNodeOutputTriggerCallback(mOutId);
    /* notify parent(s) */
    fireNodeOutputTriggerCallback(mInId);

    calculateRandomWaitTime();
    mActive = false;
    mFired = true;
  }
}

std::optional<std::map<std::string, std::string>> RandomWaitNode::exportData() {
  std::map<std::string, std::string> data{};
  data["random-wait-min-time"] = std::to_string(mMinWaitTime);
  data["random-wait-max-time"] = std::to_string(mMaxWaitTime);
  return data;
}

void RandomWaitNode::importData(std::map<std::string, std::string> data) {
  mMinWaitTime = std::stof(data["random-wait-min-time"]);
  mMaxWaitTime = std::stof(data["random-wait-max-time"]);

  calculateRandomWaitTime();
}
