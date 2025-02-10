#include "GraphEditor.h"
#include <limits>

#include "VkRenderData.h"
#include "ModelInstanceCamData.h"

#include "Logger.h"

bool GraphEditor::getShowEditor() {
  return mShowEditor;
}

void GraphEditor::closeEditor() {
  mShowEditor = false;
  mBehaviorManager.reset();
}

std::string GraphEditor::getCurrentEditedTreeName() {
  if (!mBehaviorManager) {
    return std::string();
  }

  return mBehaviorManager->getBehaviorData()->bdName;
}

void GraphEditor::createEmptyGraph() {
  mBehaviorManager = std::make_shared<SingleInstanceBehavior>();

  mFireNodeOutputCallbackFunctionFunction = [this](int nodeId) { mBehaviorManager->updateNodeStatus(nodeId); };
  mNodeFactory = std::make_shared<GraphNodeFactory>(mFireNodeOutputCallbackFunctionFunction);

  std::shared_ptr<BehaviorData> behaviorData = mBehaviorManager->getBehaviorData();

  /* add a root node on a new graph */
  behaviorData->bdGraphNodes.emplace_back(mNodeFactory->makeNode(graphNodeType::root, 0));
}

void GraphEditor::loadData(std::shared_ptr<BehaviorData> data) {
  mBehaviorManager = std::make_shared<SingleInstanceBehavior>();

  mFireNodeOutputCallbackFunctionFunction = [this](int nodeId) { mBehaviorManager->updateNodeStatus(nodeId); };
  mNodeFactory = std::make_shared<GraphNodeFactory>(mFireNodeOutputCallbackFunctionFunction);

  /* update node fire callback, may be invalidated by a std::move() */
  for (auto& node : data->bdGraphNodes) {
    node->setNodeOutputTriggerCallback(mFireNodeOutputCallbackFunctionFunction);
  }
  mBehaviorManager->setBehaviorData(data);

  ImNodes::LoadCurrentEditorStateFromIniString(data->bdEditorSettings.c_str(), data->bdEditorSettings.size());
  mShowEditor = true;
}

std::shared_ptr<SingleInstanceBehavior> GraphEditor::getData() {
  if (!mBehaviorManager) {
    return nullptr;
  }
  return mBehaviorManager;
}

void GraphEditor::updateGraphNodes(float deltaTime) {
  if (!mBehaviorManager) {
    Logger::log(1, "%s error: no data loaded\n", __FUNCTION__);
    return;
  }

  /* do not re-trigger root if no active node is left */
  mBehaviorManager->update(deltaTime, false);
}

int GraphEditor::findNextFreeNodeId() {
  std::shared_ptr<BehaviorData> behavior = mBehaviorManager->getBehaviorData();
  std::vector<int> nodeIds;
  for (const auto& node: behavior->bdGraphNodes) {
    nodeIds.emplace_back(node->getNodeId());
  }
  std::sort(nodeIds.begin(), nodeIds.end());

  const auto firstFreeNodeIter = std::adjacent_find(nodeIds.begin(), nodeIds.end(), [](int i, int j) { return i + 1 != j; });
  int firstFreeNodeId;
  if (firstFreeNodeIter != nodeIds.end()) {
    firstFreeNodeId = std::distance(nodeIds.begin(), firstFreeNodeIter) + 1;
  } else {
    firstFreeNodeId = nodeIds.size();
  }
  Logger::log(1, "%s: using free node id %i\n" , __FUNCTION__, firstFreeNodeId);
  return firstFreeNodeId;
}

int GraphEditor::findNextFreeLinkId() {
  std::shared_ptr<BehaviorData> behavior = mBehaviorManager->getBehaviorData();

  if (behavior->bdGraphLinks.empty()) {
    return LINK_ID_RANGE_START;
  }

  std::vector<int> linkIds;
  for (const auto& link: behavior->bdGraphLinks) {
    linkIds.emplace_back(link.first);
  }
  std::sort(linkIds.begin(), linkIds.end());

  /* first element is already higher than range start due to deletions, restart counting */
  if ((*linkIds.begin()) != LINK_ID_RANGE_START) {
    return LINK_ID_RANGE_START;
  }

  const auto firstFreeLinkIter = std::adjacent_find(linkIds.begin(), linkIds.end(), [](int i, int j) { return i + 1 != j; });
  int firstFreeLinkId;
  if (firstFreeLinkIter != linkIds.end()) {
    firstFreeLinkId = std::distance(linkIds.begin(), firstFreeLinkIter) + LINK_ID_RANGE_START + 1;
  } else {
    firstFreeLinkId = linkIds.size() + LINK_ID_RANGE_START;
  }
  Logger::log(1, "%s: using free link id %i\n" , __FUNCTION__, firstFreeLinkId);
  return firstFreeLinkId;
}

void GraphEditor::createNodeEditorWindow(VkRenderData& renderData, ModelInstanceCamData& modInstCamData) {
  if (!mShowEditor) {
    return;
  }
  if (!mBehaviorManager) {
    return;
  }

  std::shared_ptr<BehaviorData> behavior = mBehaviorManager->getBehaviorData();

  ImGui::SetNextWindowBgAlpha(0.5f);
  ImGui::SetNextWindowSizeConstraints(ImVec2(640, 480),
    ImVec2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()));

  std::string editorTitle = "Node Tree Template Editor - " + behavior->bdName;
  ImGui::Begin(editorTitle.c_str(), &mShowEditor);
  ImNodes::BeginNodeEditor();

  ImNodes::PushColorStyle(
    ImNodesCol_TitleBar, IM_COL32(11, 109, 191, 255));
  ImNodes::PushColorStyle(
    ImNodesCol_TitleBarSelected, IM_COL32(81, 148, 204, 255));
  ImNodes::PushColorStyle(
    ImNodesCol_TitleBarHovered, IM_COL32(141, 188, 244, 255));

  const bool openPopup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImNodes::IsEditorHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

  for (const auto& node: behavior->bdGraphNodes) {
    node->draw(modInstCamData);
  }

  for (const auto& link : behavior->bdGraphLinks) {
    ImNodes::Link(link.first, link.second.first, link.second.second);
  }

  /* pop title bar styles */
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();

  /* must be called right before EndNodeEditor */
  ImNodes::MiniMap();

  ImNodes::EndNodeEditor();

  /* popup menu, needs infos about hovered node */
  if (openPopup) {
    /* check for hovered node (must be done OUTSIDE ImNodes editor) */
    if (ImNodes::IsNodeHovered(&mHoveredNodeId)) {
      graphNodeType nodeType = graphNodeType::none;
      const auto node = std::find_if(behavior->bdGraphNodes.begin(), behavior->bdGraphNodes.end(),
                                     [&](std::shared_ptr<GraphNodeBase> node) { return node->getNodeId() == mHoveredNodeId; } );
      if (node != behavior->bdGraphNodes.end()) {
        nodeType = (*node)->getNodeType();
      }
      if (nodeType != graphNodeType::root) {
        ImGui::OpenPopup("change node");
      }
    } else {
      ImGui::OpenPopup("add node");
    }
  }

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
  if (ImGui::BeginPopup("add node")) {
    const ImVec2 clickPos = ImGui::GetMousePosOnOpeningCurrentPopup();
    ImGui::SeparatorText("Add Node");
    ImGui::Spacing();
    /* skip root node, added at start */
    for (graphNodeType nodeType = graphNodeType::root + 1; nodeType < graphNodeType::NUM; ++nodeType) {
      if (ImGui::Selectable(mNodeFactory->getNodeTypeName(nodeType).c_str())) {
        int nodeId = findNextFreeNodeId();
        std::shared_ptr<GraphNodeBase> newNode = behavior->bdGraphNodes.emplace_back(mNodeFactory->makeNode(nodeType, nodeId));

        /* add callback function for instance chaning nodes */
        if (nodeType == graphNodeType::instanceMovement || nodeType == graphNodeType::action ||
            nodeType == graphNodeType::faceAnim || nodeType == graphNodeType::headAmin ||
            nodeType == graphNodeType::randomNavigation) {
          newNode->setNodeActionCallback(behavior->bdNodeActionCallbackFunction);
        }
        ImNodes::SetNodeScreenSpacePos(nodeId, clickPos);
      }
      ImGui::Spacing();
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
  if (ImGui::BeginPopup("change node")) {
    const auto node = std::find_if(behavior->bdGraphNodes.begin(), behavior->bdGraphNodes.end(),
                                   [&](std::shared_ptr<GraphNodeBase> node) { return node->getNodeId() == mHoveredNodeId; } );
    std::shared_ptr<GraphNodeBase> selectedNode = nullptr;
    graphNodeType nodeType = graphNodeType::none;
    if (node != behavior->bdGraphNodes.end()) {
      selectedNode = (*node);
      nodeType = selectedNode->getNodeType();
    }
    ImGui::SeparatorText("Change Node");
    ImGui::Spacing();
    {
      /* save state as we might disable the node */
      const bool nodeIsNotActive = selectedNode && !selectedNode->isActive();
      if (nodeIsNotActive) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Selectable("Deactivate")) {
        if (selectedNode && selectedNode->isActive()) {
          selectedNode->deactivate();
        }
      }
      if (nodeIsNotActive) {
        ImGui::EndDisabled();
      }
    }

    {
      const bool nodeIstActive = selectedNode && selectedNode->isActive();
      if (nodeIstActive) {
        ImGui::BeginDisabled();
      }
      if (ImGui::Selectable("Delete")) {
        if (selectedNode && !selectedNode->isActive()) {
          int nodeId = selectedNode->getNodeId();
          for (auto iter = behavior->bdGraphLinks.begin(); iter != behavior->bdGraphLinks.end(); ) {
            if ((*iter).second.first / 1000 == nodeId || (*iter).second.second / 1000 == nodeId) {
              Logger::log(1, "%s: removed link from node %i\n", __FUNCTION__, nodeId);
              iter = behavior->bdGraphLinks.erase(iter);
            } else {
              ++iter;
            }
          }

          behavior->bdGraphNodes.erase(node);
        }
      }
      if (nodeIstActive) {
        ImGui::EndDisabled();
      }
    }

    {
      const bool nodeIstActive = selectedNode && selectedNode->isActive();
      if (nodeIstActive) {
        ImGui::BeginDisabled();
      }

      if (nodeType == graphNodeType::selector || nodeType == graphNodeType::sequence) {
        ImGui::SeparatorText("Change Pins");
        if (ImGui::Selectable("Add Output Pin")) {
          if (node != behavior->bdGraphNodes.end()) {
            (*node)->addOutputPin();
          }
        }

        const int numOutPins = (*node)->getNumOutputPins();
        if (numOutPins == 2) {
          ImGui::BeginDisabled();
        }
        if (ImGui::Selectable("Remove Output Pin")) {
          if (node != behavior->bdGraphNodes.end()) {
            int deletedPin = (*node)->delOutputPin();
            for (auto iter = behavior->bdGraphLinks.begin(); iter != behavior->bdGraphLinks.end(); ) {
              if ((*iter).second.first == deletedPin) {
                Logger::log(1, "%s: removed link for output pin %i\n", __FUNCTION__, deletedPin);
                iter = behavior->bdGraphLinks.erase(iter);
              } else {
                ++iter;
              }
            }
          }
        }
        if (numOutPins == 2) {
          ImGui::EndDisabled();
        }
      }
      if (nodeIstActive) {
        ImGui::EndDisabled();
      }
    }
    ImGui::EndPopup();
  }
  ImGui::PopStyleVar();

  /* check for new links - startId is always the output, endId the input */
  int startId, endId;
  if (ImNodes::IsLinkCreated(&startId, &endId)) {
    int linkId = findNextFreeLinkId();
    behavior->bdGraphLinks[linkId] = (std::make_pair(startId, endId));
    Logger::log(1, "%s: created link %i from %i to %i\n", __FUNCTION__, linkId, startId, endId);
  }

  /* check for deleted links */
  int linkId;
  if (ImNodes::IsLinkDestroyed(&linkId)) {
    behavior->bdGraphLinks.erase(linkId);
    Logger::log(1, "%s: deleted link %i\n", __FUNCTION__, linkId);
  }

  /* save positions in every frame - may be overkill, but best idea so far */
  behavior->bdEditorSettings = ImNodes::SaveCurrentEditorStateToIniString();

  ImGui::End();
}
