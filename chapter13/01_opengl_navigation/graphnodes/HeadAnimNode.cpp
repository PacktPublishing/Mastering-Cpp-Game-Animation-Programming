#include "HeadAnimNode.h"

#include <algorithm>
#include <string>
#include <vector>

#include "Logger.h"

HeadAnimNode::HeadAnimNode(int nodeId) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;

  resetTimes();
}

std::shared_ptr<GraphNodeBase> HeadAnimNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new HeadAnimNode(*this));
}

void HeadAnimNode::draw(ModelInstanceCamData modInstCamData) {
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  /* In pin */
  ImNodes::BeginInputAttribute(mInId);
  ImGui::Text("in");
  ImNodes::EndInputAttribute();

  /* head move anim */
  if (mActive) {
    ImGui::BeginDisabled();
  }

  ImGui::Checkbox("Set Left/Right Head Anim:", &mSetLeftRightHeadAnim);

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    if (!mSetLeftRightHeadAnim) {
      mHeadMoveLeftRightStartWeight = 0.0f;
      mHeadMoveLeftRightEndWeight = 0.0f;
      mHeadMoveLeftRightBlendTime = 1.0f;

      mCurrentLeftRightBlendTime = mHeadMoveLeftRightBlendTime;
    }
  }

  if (!mSetLeftRightHeadAnim) {
    ImGui::BeginDisabled();
  }

  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Start Weight Left/Right:");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##LeftRightStart", &mHeadMoveLeftRightStartWeight, -1.0f, 1.0f, "%.3f", flags);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("End Weight Left/Right:  ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##LeftRightEnd", &mHeadMoveLeftRightEndWeight, -1.0f, 1.0f, "%.3f", flags);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Left/Right Blend Time:  ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##BlendTime", &mHeadMoveLeftRightBlendTime, 0.0f, 10.0f, "%.3fs", flags);

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    mCurrentLeftRightBlendTime = mHeadMoveLeftRightBlendTime;
  }

  ImGui::Text("Left: %4.2fs", mCurrentLeftRightBlendTime);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();


  if (!mSetLeftRightHeadAnim) {
    ImGui::EndDisabled();
  }

  ImGui::Checkbox("Set Up/Down Head Anim:   ", &mSetUpDownHeadAnim);

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    if (!mSetUpDownHeadAnim) {
      mHeadMoveUpDownStartWeight = 0.0f;
      mHeadMoveUpDownEndWeight = 0.0f;
      mHeadMoveUpDownBlendTime = 1.0f;

      mCurrentUpDownBlendTime = mHeadMoveUpDownBlendTime;
    }
  }

  if (!mSetUpDownHeadAnim) {
    ImGui::BeginDisabled();
  }

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Start Weight Up/Down:   ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##UpDownStart", &mHeadMoveUpDownStartWeight, -1.0f, 1.0f, "%.3f", flags);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("End Weight Up/Down:     ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##UpDownEnd", &mHeadMoveUpDownEndWeight, -1.0f, 1.0f, "%.3f", flags);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Up/Down Blend Time:     ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##BlendTime", &mHeadMoveUpDownBlendTime, 0.0f, 10.0f, "%.3fs", flags);

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    mCurrentUpDownBlendTime = mHeadMoveUpDownBlendTime;
  }

  ImGui::Text("Up: %4.2fs", mCurrentUpDownBlendTime);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();


  if (!mSetUpDownHeadAnim) {
    ImGui::EndDisabled();
  }

  if (mActive) {
    ImGui::EndDisabled();
  }

  /* Out pin */
  ImNodes::BeginOutputAttribute(mOutId);
  if (mFired) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(0, 255, 0, 255)));
  }
  ImGui::Text("                                 out");
  if (mFired) {
    ImGui::PopStyleColor();
  }
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void HeadAnimNode::update(float deltaTime) {
  if (!mActive) {
    return;
  }

  if (mSetLeftRightHeadAnim) {
    mCurrentLeftRightBlendTime -= deltaTime;
  } else {
    mCurrentLeftRightBlendTime = 0.0f;
  }

  /* time diff goes from 0.0f to 1.0f */
  float leftRightTimeDiff = 0.0f;
  if (mHeadMoveLeftRightBlendTime != 0.0f) {
    leftRightTimeDiff = std::clamp(mCurrentLeftRightBlendTime / mHeadMoveLeftRightBlendTime, 0.0f, 1.0f);
  }

  float leftRightWeightDiff = mHeadMoveLeftRightEndWeight - mHeadMoveLeftRightStartWeight;
  float leftRightCurrentWeight = mHeadMoveLeftRightEndWeight - leftRightWeightDiff * leftRightTimeDiff;

  if (mSetUpDownHeadAnim) {
    mCurrentUpDownBlendTime -= deltaTime;
  } else {
    mCurrentUpDownBlendTime = 0.0f;
  }

  float upDownTimeDiff = 0.0f;
  if (mHeadMoveUpDownBlendTime != 0.0f) {
    upDownTimeDiff = std::clamp(mCurrentUpDownBlendTime / mHeadMoveUpDownBlendTime, 0.0f, 1.0f);
  }

  float upDownWeightDiff = mHeadMoveUpDownEndWeight - mHeadMoveUpDownStartWeight;
  float upDownCurrentWeight = mHeadMoveUpDownEndWeight - upDownWeightDiff * upDownTimeDiff;

  /* update values */
  instanceUpdateType updateType = instanceUpdateType::headAnim;
  nodeCallbackVariant result;
  bool extra = false;
  result = glm::vec2(leftRightCurrentWeight, upDownCurrentWeight);
  fireNodeActionCallback(getNodeType(), updateType, result, extra);

  if (mCurrentLeftRightBlendTime <= 0.0f && mCurrentUpDownBlendTime <= 0.0f) {
    /* notify child(s) */
    fireNodeOutputTriggerCallback(mOutId);
    /* notify parent(s) */
    fireNodeOutputTriggerCallback(mInId);

    resetTimes();

    mActive = false;
    mFired = true;
  }
}

void HeadAnimNode::activate() {
  mActive = true;
}

void HeadAnimNode::deactivate(bool informParentNodes) {
  if (!mActive) {
    return;
  }

  mActive = false;
  mFired = false;

  resetTimes();

  if (informParentNodes) {
    /* inform parent that we are done */
    fireNodeOutputTriggerCallback(mInId);
  }
}

void HeadAnimNode::resetTimes() {
  mCurrentLeftRightBlendTime = mHeadMoveLeftRightBlendTime;
  mCurrentLeftRightBlendValue = mHeadMoveLeftRightStartWeight;
  mCurrentUpDownBlendTime = mHeadMoveUpDownBlendTime;
  mCurrentUpDownBlendValue = mHeadMoveUpDownStartWeight;
}

std::optional<std::map<std::string, std::string>> HeadAnimNode::exportData() {
  std::map<std::string, std::string> data;
  if (mSetLeftRightHeadAnim) {
    data["head-anim-left-right"] = std::to_string(mSetLeftRightHeadAnim);
    data["head-anim-left-right-start-weight"] = std::to_string(mHeadMoveLeftRightStartWeight);
    data["head-anim-left-right-end-weight"] = std::to_string(mHeadMoveLeftRightEndWeight);
    data["head-anim-left-right-blend-time"] = std::to_string(mHeadMoveLeftRightBlendTime);
  }

  if (mSetUpDownHeadAnim) {
    data["head-anim-up-down"] = std::to_string(mSetUpDownHeadAnim);
    data["head-anim-up-down-start-weight"] = std::to_string(mHeadMoveUpDownStartWeight);
    data["head-anim-up-down-end-weight"] = std::to_string(mHeadMoveUpDownEndWeight);
    data["head-anim-up-down-blend-time"] = std::to_string(mHeadMoveUpDownBlendTime);
  }

  if (data.empty()) {
    return {};
  }
  return data;
}

void HeadAnimNode::importData(std::map<std::string, std::string> data) {
  if (data.count("head-anim-left-right") > 0) {
    mSetLeftRightHeadAnim = true;
    mHeadMoveLeftRightStartWeight = std::stof(data["head-anim-left-right-start-weight"]);
    mHeadMoveLeftRightEndWeight = std::stof(data["head-anim-left-right-end-weight"]);
    mHeadMoveLeftRightBlendTime = std::stof(data["head-anim-left-right-blend-time"]);
  }

  if (data.count("head-anim-up-down") > 0) {
    mSetUpDownHeadAnim = true;
    mHeadMoveUpDownStartWeight = std::stof(data["head-anim-up-down-start-weight"]);
    mHeadMoveUpDownEndWeight = std::stof(data["head-anim-up-down-end-weight"]);
    mHeadMoveUpDownBlendTime = std::stof(data["head-anim-up-down-blend-time"]);
  }

  resetTimes();
}


