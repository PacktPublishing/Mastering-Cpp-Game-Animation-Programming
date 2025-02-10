#pragma once

#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <string>
#include <map>

#include "Callbacks.h"

/* fordward declaration */
class GraphNodeBase;

struct BehaviorData {
  std::vector<std::shared_ptr<GraphNodeBase>> bdGraphNodes{};
  std::unordered_map<int, std::pair<int, int>> bdGraphLinks{};

  /* may be duplicate (also in std::map), but needed sometimes */
  std::string bdName;

  /* store node positions while in editor */
  std::string bdEditorSettings;

  nodeActionCallback bdNodeActionCallbackFunction;
};

/* enhanced struct to get extra data */
struct PerNodeImportData {
  int nodeId;
  graphNodeType nodeType;
  std::map<std::string, std::string> nodeProperties{};
};

struct ExtendedBehaviorData : BehaviorData {
  std::vector<PerNodeImportData> nodeImportData{};
};
