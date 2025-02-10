#pragma once

#include <vector>
#include <memory>
#include <unordered_map>
#include <tuple>

#include "GraphNodeFactory.h"
#include "Enums.h"
#include "Callbacks.h"
#include "SingleInstanceBehavior.h"

/* forward declaration against circular dependencies */
struct VkRenderData;
struct ModelInstanceCamData;


class GraphEditor {
  public:
    void loadData(std::shared_ptr<BehaviorData> data);
    std::shared_ptr<SingleInstanceBehavior> getData();

    bool getShowEditor();
    std::string getCurrentEditedTreeName();
    void closeEditor();
    void createEmptyGraph();

    void updateGraphNodes(float deltaTime);
    void createNodeEditorWindow(VkRenderData &renderData, ModelInstanceCamData &modInstCamData);

  private:
    void updateNodeStatus(int pinId);
    int findNextFreeNodeId();
    int findNextFreeLinkId();

    fireNodeOutputCallback mFireNodeOutputCallbackFunctionFunction;

    std::shared_ptr<GraphNodeFactory> mNodeFactory = nullptr;;
    std::shared_ptr<SingleInstanceBehavior> mBehaviorManager = nullptr;

    bool mShowEditor = false;
    int mHoveredNodeId;
    const int LINK_ID_RANGE_START = 10'000;
};
