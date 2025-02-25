#include "ActionNode.h"

#include "Logger.h"

ActionNode::ActionNode(int nodeId) : GraphNodeBase(nodeId) {
  /* attributes: nodeId  * 1000, ascending */
  int id = nodeId * 1000;
  mInId = id;
  mStaticIdStart = id + 100;
  mOutId = id + 200;
}

std::shared_ptr<GraphNodeBase> ActionNode::clone() {
  return std::shared_ptr<GraphNodeBase>(new ActionNode(*this));
}

void ActionNode::draw(ModelInstanceCamData modInstCamData) {
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
  ImGui::Checkbox("Set Action", &mSetState);
  if (!mSetState) {
    ImGui::BeginDisabled();
  }
  ImGui::PushItemWidth(100.0f);
  if (ImGui::BeginCombo("##ActionNodeStateCombo",
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

  /* Out pin */
  ImNodes::BeginOutputAttribute(mOutId);
  ImGui::Text("                 out");
  ImNodes::EndOutputAttribute();

  ImNodes::EndNode();
}

void ActionNode::activate() {
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

  /* notify childs */
  fireNodeOutputTriggerCallback(mOutId);
}

std::optional<std::map<std::string, std::string>> ActionNode::exportData() {
  std::map<std::string, std::string> data;
  if (mSetState) {
    data["action-move-state"] = std::to_string(static_cast<int>(mMoveState));
  }
  if (data.empty()) {
    return {};
  }
  return data;
}

void ActionNode::importData(std::map<std::string, std::string> data) {
  if (data.count("action-move-state") > 0) {
    mSetState = true;
    mMoveState = static_cast<moveState>(std::stoi(data["action-move-state"]));
  }
}
