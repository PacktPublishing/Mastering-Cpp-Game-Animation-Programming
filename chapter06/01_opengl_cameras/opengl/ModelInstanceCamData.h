/* separate settings file to avoid cicrula dependecies */
#pragma once

#include <memory>
#include <vector>
#include <map>
#include <unordered_map>

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
  std::map<std::string, std::vector<std::shared_ptr<AssimpInstance>>> micAssimpInstancesPerModel{};
  int micSelectedInstance = 0;

  std::shared_ptr<AssimpSettingsContainer> micSettingsContainer;

  std::vector<std::shared_ptr<Camera>> micCameras{};
  int micSelectedCamera = 0;

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
  setConfigDirtyCallbackFunction micSetConfigDirtyCallbackFunction;
  getConfigDirtyCallbackFunction micGetConfigDirtyCallbackFunction;

  cameraCloneCallback micCameraCloneCallbackFunction;
  cameraDeleteCallback micCameraDeleteCallbackFunction;
  cameraNameCheckCallback micCameraNameCheckCallback;

  std::unordered_map<cameraType, std::string> micCameraTypeMap{};
  std::unordered_map<cameraProjection, std::string> micCameraProjectionMap{};
};
