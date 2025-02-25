#include "InstanceMovementNode.h"

#include <glm/gtx/string_cast.hpp>

InstanceMovementNode::InstanceMovementNode(int nodeId) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;
}

std::shared_ptr<GraphNodeBase> InstanceMovementNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new InstanceMovementNode(*this));
}

void InstanceMovementNode::draw(ModelInstanceCamData modInstCamData) {
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  /* In pin */
  ImNodes::BeginInputAttribute(mInId);
  ImGui::Text("in");
  ImNodes::EndInputAttribute();

  /* New state */
  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Set State", &mSetState);
  if (!mSetState) {
    ImGui::BeginDisabled();
  }
  ImGui::PushItemWidth(100.0f);
  if (ImGui::BeginCombo("##InstanceNodeStateCombo",
    modInstCamData.micMoveStateMap.at(mMoveState).c_str())) {
    for (int i = 0; i < static_cast<int>(moveState::NUM); ++i) {
      const bool isSelected = (static_cast<int>(mMoveState) == i);
      if (ImGui::Selectable(modInstCamData.micMoveStateMap[static_cast<moveState>(i)].c_str(), isSelected)) {
        mMoveState = static_cast<moveState>(i);
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopItemWidth();
  if (!mSetState) {
    ImGui::EndDisabled();
  }
  ImNodes::EndStaticAttribute();

  ImGui::NewLine();

  /* New movement direction */
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Set Direction", &mSetMoveDirection);
  if (!mSetMoveDirection) {
    ImGui::BeginDisabled();
  }
  ImGui::PushItemWidth(100.0f);
  if (ImGui::BeginCombo("##DirComboNode",
    modInstCamData.micMoveDirectionMap.at(mMoveDir).c_str())) {
    for (int i = 0; i < modInstCamData.micMoveDirectionMap.size(); ++i) {
      /* skip empty directions, and the 'any'  */
      if (modInstCamData.micMoveDirectionMap[static_cast<moveDirection>(i)].empty() ||
        static_cast<moveDirection>(i) == moveDirection::any) {
        continue;
        }
        const bool isSelected = (static_cast<int>(mMoveDir) == i);
      if (ImGui::Selectable(modInstCamData.micMoveDirectionMap[static_cast<moveDirection>(i)].c_str(), isSelected)) {
        mMoveDir = static_cast<moveDirection>(i);
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopItemWidth();
  if (!mSetMoveDirection) {
    ImGui::EndDisabled();
  }
  ImNodes::EndStaticAttribute();

  ImGui::NewLine();

  /* New speed */
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Set Speed", &mSetSpeed);
  if (!mSetSpeed) {
    ImGui::BeginDisabled();
  }
  if (!mRandomSpeed) {
    ImGui::Text("Speed:");
  } else {
    ImGui::Text("Min:  ");
  }
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##SpeedFloatMin", &mMinSpeed, 0.0f, 2.0f, "%.2f", flags);
  ImGui::PopItemWidth();

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    calculateSpeed();
  }

  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  if (mRandomSpeed) {
    ImGui::Text("Max:  ");
    ImGui::SameLine();
    ImGui::PushItemWidth(100.0f);
    ImGui::SliderFloat("##SpeedFloatMax", &mMaxSpeed, 0.0f, 2.0f, "%.2f", flags);

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      calculateSpeed();
    }
  }
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Random Speed", &mRandomSpeed);
  ImNodes::EndStaticAttribute();

  /* adjust when switching checkbox */
  if (mRandomSpeedChanged != mRandomSpeed) {
    calculateSpeed();
    mRandomSpeedChanged = mRandomSpeed;
  }

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Current Speed:  %4.2f", mSpeed);
  if (!mSetSpeed) {
    ImGui::EndDisabled();
  }
  ImNodes::EndStaticAttribute();

  ImGui::NewLine();

  /* New rotation */
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Set Rotation", &mSetRotation);
  if (!mSetRotation) {
    ImGui::BeginDisabled();
  }
  if (!mRandomRot) {
    ImGui::Text("Rot: ");
  } else {
    ImGui::Text("Min: ");
  }
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##RotFloatMin", &mMinRot, -180.0f, 180.0f, "%.2f", flags);
  ImGui::PopItemWidth();

  if (ImGui::IsItemDeactivatedAfterEdit()) {
    calculateRotation();
  }

  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  if (mRandomRot) {
    ImGui::Text("Max: ");
    ImGui::SameLine();
    ImGui::PushItemWidth(100.0f);
    ImGui::SliderFloat("##RotFloatMax", &mMaxRot, -180.0f, 180.0f, "%.2f", flags);

    if (ImGui::IsItemDeactivatedAfterEdit()) {
      calculateRotation();
    }
  }
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Random Rotataion", &mRandomRot);
  ImNodes::EndStaticAttribute();

  /* adjust when switching checkbox */
  if (mRandomRotChanged != mRandomRot) {
    calculateRotation();
    mRandomRotChanged = mRandomRot;
  }

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Relative Rotation", &mRelativeRot);
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Current Rot: %3.2f", mRotation);
  if (!mSetRotation) {
    ImGui::EndDisabled();
  }
  ImNodes::EndStaticAttribute();

  /* New position */
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Checkbox("Set Position", &mSetPosition);
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    if (!mSetPosition) {
      mPosition = glm::vec3(0.0f);
    }
  }

  if (!mSetPosition) {
    ImGui::BeginDisabled();
  }

  ImGui::Text("Pos: ");
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##PosXFloat", &mPosition.x, -75.0f, 75.0f, "%.3f", flags);
  ImGui::SliderFloat("##PosYFloat", &mPosition.y, -75.0f, 75.0f, "%.3f", flags);
  ImGui::SliderFloat("##PosZFloat", &mPosition.z, -75.0f, 75.0f, "%.3f", flags);
  ImGui::PopItemWidth();

  if (!mSetPosition) {
    ImGui::EndDisabled();
  }
  ImNodes::EndStaticAttribute();

  /* Out pin */
  ImNodes::BeginOutputAttribute(mOutId);
  ImGui::Text("                 out");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void InstanceMovementNode::activate() {
  instanceUpdateType updateType = instanceUpdateType::none;
  nodeCallbackVariant result;
  bool extra = false;
  if (mSetState) {
    result = mMoveState;
    updateType = instanceUpdateType::moveState;
    fireNodeActionCallback(getNodeType(), updateType, result, extra);

    Logger::log(2, "%s: node '%s' (id %i) has set movement state to %i\n", __FUNCTION__,
                getNodeName().c_str(), updateType, static_cast<int>(mMoveState));
  }

  if (mSetMoveDirection) {
    result = mMoveDir;
    updateType = instanceUpdateType::moveDirection;
    fireNodeActionCallback(getNodeType(), updateType, result, extra);

    Logger::log(2, "%s: node '%s' (id %i) has set movement direction to %i\n", __FUNCTION__,
                getNodeName().c_str(), updateType, static_cast<int>(mMoveDir));
  }

  if (mSetSpeed) {
    calculateSpeed();
    result = mSpeed;
    updateType = instanceUpdateType::speed;
    fireNodeActionCallback(getNodeType(), updateType, result, extra);

    Logger::log(2, "%s: node '%s' (id %i) has set speed to %4.2f\n", __FUNCTION__,
                getNodeName().c_str(), updateType, mSpeed);
  }

  if (mSetRotation) {
    calculateRotation();
    result = mRotation;
    extra = mRelativeRot;
    updateType = instanceUpdateType::rotation;
    fireNodeActionCallback(getNodeType(), updateType, result, extra);

    Logger::log(2, "%s: node '%s' (id %i) has set %s rotation to %4.2f\n", __FUNCTION__,
                getNodeName().c_str(), updateType, mRelativeRot ? "relative" : "absolute", mRotation);
  }
  if (mSetPosition) {
    result = mPosition;
    extra = false;
    updateType = instanceUpdateType::position;
    fireNodeActionCallback(getNodeType(), updateType, result, extra);

    Logger::log(2, "%s: node '%s' (id %i) has set %s position to %s\n", __FUNCTION__,
                getNodeName().c_str(), updateType, glm::to_string(mPosition).c_str());
  }

  /* notify childs */
  fireNodeOutputTriggerCallback(mOutId);
}

void InstanceMovementNode::calculateRotation() {
  if (!mRandomRot) {
    mRotation = mMinRot;
    return;
  }

  if (mMaxRot < mMinRot) {
    mMaxRot = mMinRot;
    mRotation = mMinRot;
  } else {
    mRotation = mMinRot;
    if (std::fabs(mMaxRot - mMinRot) > 0.01f) {
      mRotation = std::rand() % static_cast<int>((mMaxRot - mMinRot) * 100) + mMinRot * 100;
      if (std::fabs(mRotation) > 0.01f) {
        mRotation /= 100.0f;
      }
    }
  }
}

void InstanceMovementNode::calculateSpeed() {
  if (!mRandomSpeed) {
    mSpeed = mMinSpeed;
    return;
  }

  if (mMaxSpeed < mMinSpeed) {
    mMaxSpeed = mMinSpeed;
    mSpeed = mMinSpeed;
  } else {
    mSpeed = mMinSpeed;
    if (std::fabs(mMaxSpeed - mMinSpeed) > 0.01f) {
      mSpeed = std::rand() % static_cast<int>((mMaxSpeed - mMinSpeed) * 100) + mMinSpeed * 100;
      if (std::fabs(mSpeed) > 0.01f) {
        mSpeed /= 100.0f;
      }
    }
  }
}

std::optional<std::map<std::string, std::string>> InstanceMovementNode::exportData() {
  std::map<std::string, std::string> data;
  if (mSetState) {
    data["instance-move-state"] = std::to_string(static_cast<int>(mMoveState));
  }
  if (mSetMoveDirection)  {
    data["instance-move-direction"] = std::to_string(static_cast<int>(mMoveDir));
  }

  if (mSetRotation) {
    data["instance-min-rotation"] = std::to_string(mMinRot);
    data["instance-max-rotation"] = std::to_string(mMaxRot);
    data["instance-random-rotation"] = std::to_string(mRandomRot);
    data["instance-relative-rotation"] = std::to_string(mRelativeRot);
  }

  if (mSetSpeed)  {
    data["instance-min-speed"] = std::to_string(mMinSpeed);
    data["instance-max-speed"] = std::to_string(mMaxSpeed);
    data["instance-random-speed"] = std::to_string(mRandomSpeed);
  }

  if (mSetPosition) {
    data["instance-position-x"] = std::to_string(mPosition.x);
    data["instance-position-y"] = std::to_string(mPosition.y);
    data["instance-position-z"] = std::to_string(mPosition.z);
  }

  if (data.empty()) {
    return {};
  }
  return data;
}

void InstanceMovementNode::importData(std::map<std::string, std::string> data) {
  if (data.count("instance-move-state") > 0) {
    mSetState = true;
    mMoveState = static_cast<moveState>(std::stoi(data["instance-move-state"]));
  }

  if (data.count("instance-move-direction") > 0) {
    mSetMoveDirection = true;
    mMoveDir = static_cast<moveDirection>(std::stoi(data["instance-move-direction"]));
  }

  if (data.count("instance-min-rotation") > 0) {
    mSetRotation = true;
    mMinRot = std::stof(data["instance-min-rotation"]);
    mMaxRot = std::stof(data["instance-max-rotation"]);
    mRandomRot = std::stoi(data["instance-random-rotation"]);
    mRelativeRot = std::stoi(data["instance-relative-rotation"]);
  }

  if (data.count("instance-min-speed") > 0) {
    mSetSpeed = true;
    mMinSpeed = std::stof(data["instance-min-speed"]);
    mMaxSpeed = std::stof(data["instance-max-speed"]);
    mRandomSpeed = std::stoi(data["instance-random-speed"]);
  }

  if (data.count("instance-position-x") > 0) {
    mSetPosition = true;
    mPosition.x = std::stof(data["instance-position-x"]);
    mPosition.y = std::stof(data["instance-position-y"]);
    mPosition.z = std::stof(data["instance-position-z"]);
  }
}
