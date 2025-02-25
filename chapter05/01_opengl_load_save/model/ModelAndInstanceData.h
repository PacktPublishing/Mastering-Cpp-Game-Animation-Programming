/* separate settings file to avoid cicrula dependecies */
#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Callbacks.h"

// forward declaration
class AssimpModel;
class AssimpInstance;
class AssimpSettingsContainer;

struct ModelAndInstanceData {
  std::vector<std::shared_ptr<AssimpModel>> miModelList{};
  int miSelectedModel = 0;

  std::vector<std::shared_ptr<AssimpInstance>> miAssimpInstances{};
  std::unordered_map<std::string, std::vector<std::shared_ptr<AssimpInstance>>> miAssimpInstancesPerModel{};
  int miSelectedInstance = 0;

  /* delete models that were loaded during application runtime */
  std::unordered_set<std::shared_ptr<AssimpModel>> miPendingDeleteAssimpModels{};

  std::shared_ptr<AssimpSettingsContainer> miSettingsContainer{};

  /* callbacks */
  setWindowTitleCallback miSetWindowTitleFunction;
  getWindowTitleCallback miGetWindowTitleFunction;

  modelCheckCallback miModelCheckCallbackFunction;
  modelAddCallback miModelAddCallbackFunction;
  modelDeleteCallback miModelDeleteCallbackFunction;

  instanceAddCallback miInstanceAddCallbackFunction;
  instanceAddManyCallback miInstanceAddManyCallbackFunction;
  instanceDeleteCallback miInstanceDeleteCallbackFunction;
  instanceCloneCallback miInstanceCloneCallbackFunction;
  instanceCloneManyCallback miInstanceCloneManyCallbackFunction;

  instanceCenterCallback miInstanceCenterCallbackFunction;

  undoRedoCallback miUndoCallbackFunction;
  undoRedoCallback miRedoCallbackFunction;

  loadSaveCallback miSaveConfigCallbackFunction;
  loadSaveCallback miLoadConfigCallbackFunction;
};
