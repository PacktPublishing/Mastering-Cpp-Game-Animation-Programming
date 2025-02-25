/* separate settings file to avoid cicrula dependecies */
#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <set>
#include <unordered_set>

#include "Callbacks.h"

// forward declaration
class AssimpModel;
class AssimpInstance;
class AssimpSettingsContainer;
class Camera;

struct ModelInstanceCamData {
  std::vector<std::shared_ptr<AssimpModel>> micModelList{};
  int micSelectedModel = 0;

  std::vector<std::shared_ptr<AssimpInstance>> micAssimpInstances{};
  std::unordered_map<std::string, std::vector<std::shared_ptr<AssimpInstance>>> micAssimpInstancesPerModel{};
  int micSelectedInstance = 0;

  std::shared_ptr<AssimpSettingsContainer> micSettingsContainer{};

  std::vector<std::shared_ptr<Camera>> micCameras{};
  int micSelectedCamera = 0;

  std::unordered_map<cameraType, std::string> micCameraTypeMap{};
  std::unordered_map<cameraProjection, std::string> micCameraProjectionMap{};
  std::unordered_map<moveDirection, std::string> micMoveDirectionMap{};
  std::unordered_map<moveState, std::string> micMoveStateMap{};

  std::set<std::pair<int, int>> micInstanceCollisions{};

  /* delete models that were loaded during application runtime */
  std::unordered_set<std::shared_ptr<AssimpModel>> micPendingDeleteAssimpModels{};

  /* callbacks */
  setWindowTitleCallback micSetWindowTitleFunction;
  getWindowTitleCallback micGetWindowTitleFunction;
  setAppModeCallback micSsetAppModeCallbackFunction;

  modelCheckCallback micModelCheckCallbackFunction;
  modelAddCallback micModelAddCallbackFunction;
  modelDeleteCallback micModelDeleteCallbackFunction;

  instanceAddCallback micInstanceAddCallbackFunction;
  instanceAddManyCallback micInstanceAddManyCallbackFunction;
  instanceDeleteCallback micInstanceDeleteCallbackFunction;
  instanceCloneCallback micInstanceCloneCallbackFunction;
  instanceCloneManyCallback micInstanceCloneManyCallbackFunction;

  instanceCenterCallback micInstanceCenterCallbackFunction;

  undoRedoCallback micUndoCallbackFunction;
  undoRedoCallback micRedoCallbackFunction;

  loadSaveCallback micSaveConfigCallbackFunction;
  loadSaveCallback micLoadConfigCallbackFunction;

  newConfigCallback micNewConfigCallbackFunction;
  setConfigDirtyCallback micSetConfigDirtyCallbackFunction;
  getConfigDirtyCallback micGetConfigDirtyCallbackFunction;

  cameraCloneCallback micCameraCloneCallbackFunction;
  cameraDeleteCallback micCameraDeleteCallbackFunction;
  cameraNameCheckCallback micCameraNameCheckCallbackFunction;

  instanceGetPositionsCallback micInstanceGetPositionsCallbackFunction;

  quadTreeQueryBBoxCallback micQuadTreeQueryBBoxCallbackFunction;
  quadTreeFindAllIntersectionsCallback micQuadTreeFindAllIntersectionsCallbackFunction;
  quadTreeGetBoxesCallback micQuadTreeGetBoxesCallbackFunction;

  worldGetBoundariesCallback micWorldGetBoundariesCallbackFunction;
};
