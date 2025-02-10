#include "FaceAnimNode.h"

#include <algorithm>
#include <string>
#include <vector>

FaceAnimNode::FaceAnimNode(int nodeId) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;

  resetTimes();
}

std::shared_ptr<GraphNodeBase> FaceAnimNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new FaceAnimNode(*this));
}

void FaceAnimNode::draw(ModelInstanceCamData modInstCamData) {
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  /* In pin */
  ImNodes::BeginInputAttribute(mInId);
  ImGui::Text("in");
  ImNodes::EndInputAttribute();

  /* morph anim */
  if (mActive) {
    ImGui::BeginDisabled();
  }

  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);

  ImGui::Text("New Clip:    ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  if (ImGui::BeginCombo("##FaceAnimCombo",
    modInstCamData.micFaceAnimationNameMap.at(mFaceAnim).c_str())) {
    for (int i = 0; i < modInstCamData.micFaceAnimationNameMap.size(); ++i) {
      const bool isSelected = (static_cast<int>(mFaceAnim) == i);
      if (ImGui::Selectable(modInstCamData.micFaceAnimationNameMap.at(static_cast<faceAnimation>(i)).c_str(), isSelected)) {
        mFaceAnim = static_cast<faceAnimation>(i);
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Start Weight:");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##WeightStart", &mFaceAnimStartWeight, 0.0f, 1.0f, "%.3f", flags);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("End Weight:  ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##WeightEnd", &mFaceAnimEndWeight, 0.0f, 1.0f, "%.3f", flags);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Blend Time:  ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##BlendTime", &mFaceAnimBlendTime, 0.0f, 10.0f, "%.3fs", flags);

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    mCurrentTime = mFaceAnimBlendTime;
  }

  ImGui::Text("Left: %4.2fs", mCurrentTime);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  if (mActive) {
    ImGui::EndDisabled();
  }

  /* Out pin */
  ImNodes::BeginOutputAttribute(mOutId);
  if (mFired) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(ImColor(0, 255, 0, 255)));
  }
  ImGui::Text("                         out");
  if (mFired) {
    ImGui::PopStyleColor();
  }
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void FaceAnimNode::update(float deltaTime) {
  if (!mActive) {
    return;
  }

  mCurrentTime -= deltaTime;

  /* time diff goes from 0.0f to 1.0f */
  float morphTimeDiff = 1.0f;
  if (mFaceAnimBlendTime != 0.0f) {
    morphTimeDiff = std::clamp(mCurrentTime / mFaceAnimBlendTime, 0.0f, 1.0f);
  }

  float morphWeightDiff = mFaceAnimEndWeight - mFaceAnimStartWeight;
  float currentWeight = mFaceAnimEndWeight - morphWeightDiff * morphTimeDiff;

  instanceUpdateType updateType = instanceUpdateType::faceAnimWeight;
  nodeCallbackVariant result;
  bool extra = false;
  result = currentWeight;
  fireNodeActionCallback(getNodeType(), updateType, result, extra);

  if (mCurrentTime <= 0.0f) {
    /* notify child(s) */
    fireNodeOutputTriggerCallback(mOutId);
    /* notify parent(s) */
    fireNodeOutputTriggerCallback(mInId);

    resetTimes();

    mActive = false;
    mFired = true;
  }
}

void FaceAnimNode::activate() {
  mActive = true;

  instanceUpdateType updateType = instanceUpdateType::faceAnimIndex;
  nodeCallbackVariant result;
  bool extra = false;
  result = mFaceAnim;
  fireNodeActionCallback(getNodeType(), updateType, result, extra);
}

void FaceAnimNode::deactivate(bool informParentNodes) {
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

std::optional<std::map<std::string, std::string>> FaceAnimNode::exportData() {
  std::map<std::string, std::string> data;
  if (mFaceAnim != faceAnimation::none) {
    data["face-anim"] = std::to_string(static_cast<int>(mFaceAnim));
    data["face-anim-start-weight"] = std::to_string(mFaceAnimStartWeight);
    data["face-anim-end-weight"] = std::to_string(mFaceAnimEndWeight);
    data["face-anim-blend-time"] = std::to_string(mFaceAnimBlendTime);
  }

  if (data.empty()) {
    return {};
  }
  return data;
}

void FaceAnimNode::importData(std::map<std::string, std::string> data) {
  if (data.count("face-anim") > 0) {
    mFaceAnim = static_cast<faceAnimation>(std::stoi(data["face-anim"]));
    mFaceAnimStartWeight = std::stof(data["face-anim-start-weight"]);
    mFaceAnimEndWeight= std::stof(data["face-anim-end-weight"]);
    mFaceAnimBlendTime= std::stof(data["face-anim-blend-time"]);

    resetTimes();
  }
}

void FaceAnimNode::resetTimes() {
  mCurrentTime = mFaceAnimBlendTime;
  mCurrentBlendValue = mFaceAnimStartWeight;
}
