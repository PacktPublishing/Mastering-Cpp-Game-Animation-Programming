#include "EventNode.h"

EventNode::EventNode(int nodeId, float cooldown) : GraphNodeBase(nodeId) {
  mEventCooldown = cooldown;

  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mStaticIdStart = id + 100;
  mOutId = id + 200;
}

std::shared_ptr<GraphNodeBase> EventNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new EventNode(*this));
}

void EventNode::draw(ModelInstanceCamData modInstCamData) {
  ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp;

  ImNodes::BeginNode(getNodeId());

  ImNodes::BeginNodeTitleBar();
  ImGui::TextUnformatted(getFormattedNodeName().c_str());
  ImNodes::EndNodeTitleBar();

  bool eventWasTriggered = mEventTriggered;
  int staticIds = mStaticIdStart;
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Event to wait for:");
  if (eventWasTriggered) {
    ImGui::BeginDisabled();
  }
  ImGui::PushItemWidth(200.0f);
  if (ImGui::BeginCombo("##NodeEventCombo",
    modInstCamData.micNodeUpdateMap.at(mTriggerEvent).c_str())) {
    for (int i = 0; i < static_cast<int>(nodeEvent::NUM); ++i) {
      const bool isSelected = (static_cast<int>(mTriggerEvent) == i);
      if (ImGui::Selectable(modInstCamData.micNodeUpdateMap[static_cast<nodeEvent>(i)].c_str(), isSelected)) {
        mTriggerEvent = static_cast<nodeEvent>(i);
      }

      if (isSelected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::PopItemWidth();
  if (eventWasTriggered) {
    ImGui::EndDisabled();
  }
  ImNodes::EndStaticAttribute();

  if (eventWasTriggered) {
    ImGui::BeginDisabled();
  }
  ImNodes::BeginStaticAttribute(staticIds++);
  ImGui::Text("Cooldown: ");
  ImGui::SameLine();
  ImGui::PushItemWidth(100.0f);
  ImGui::SliderFloat("##CooldownFloat", &mEventCooldown, 0.0f, 25.0f, "%.3fs", flags);
  ImGui::Text("Left: %4.2fs", mCooldown);
  ImGui::PopItemWidth();
  ImNodes::EndStaticAttribute();

  ImNodes::BeginStaticAttribute(staticIds++);
  if (ImGui::Button("Trigger Test")) {
    handleEvent();
  }
  ImNodes::EndStaticAttribute();
  if (eventWasTriggered) {
    ImGui::EndDisabled();
  }

  /* Out pin */
  ImNodes::BeginOutputAttribute(mOutId);
  ImGui::Text("                        out");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

bool EventNode::listensToEvent(nodeEvent event) {
  if (event == mTriggerEvent) {
    return true;
  }

  return false;
}

void EventNode::handleEvent() {
  if (mCooldown > 0.0f && mEventTriggered) {
    return;
  }
  fireNodeOutputTriggerCallback(mOutId);
  mCooldown = mEventCooldown;
  mEventTriggered = true;
}

void EventNode::update(float deltaTime) {
  if (mCooldown > 0.0f) {
    mCooldown -= deltaTime;
  } else {
    mEventTriggered = false;
    mCooldown = 0.0f;
  }
}

void EventNode::deactivate(bool informParentNodes) {
  mEventTriggered = false;
  mCooldown = 0.0f;
}

std::optional<std::map<std::string, std::string>> EventNode::exportData() {
  std::map<std::string, std::string> data{};
  data["event-type"] = std::to_string(static_cast<int>((mTriggerEvent)));
  data["event-cooldown"] = std::to_string(mEventCooldown);
  return data;
}

void EventNode::importData(std::map<std::string, std::string> data) {
  mTriggerEvent = static_cast<nodeEvent>(std::stoi(data["event-type"]));
  mEventCooldown = std::stof(data["event-cooldown"]);
  mCooldown = 0.0f;
}
