#include "AssimpSettingsContainer.h"
#include "Logger.h"

AssimpSettingsContainer::AssimpSettingsContainer(std::shared_ptr<AssimpInstance> nullInstance) {
  mNullInstance = nullInstance;
}

void AssimpSettingsContainer::removeStacks() {
  /* kill undo and redo, i.e., when loading a config */
  std::stack<UndoRedoSettings> emptyUndoStack = std::stack<UndoRedoSettings>();
  mUndoStack.swap(emptyUndoStack);

  removeRedoStack();
}

void AssimpSettingsContainer::removeRedoStack() {
  std::stack<UndoRedoSettings> emptyRedoStack = std::stack<UndoRedoSettings>();
  mRedoStack.swap(emptyRedoStack);
}

int AssimpSettingsContainer::getCurrentInstance() {
  if (!mUndoStack.empty()) {
    Logger::log(1, "%s: current undo instance %i\n", __FUNCTION__, mUndoStack.top().ursSelectedInstance);
    return mUndoStack.top().ursSelectedInstance;
  }
  if (!mRedoStack.empty()) {
    Logger::log(1, "%s: current redo instance %i\n", __FUNCTION__, mRedoStack.top().ursSavedSelectedInstance);
    return mRedoStack.top().ursSavedSelectedInstance;
  }
  /* fallback to null instance */
  Logger::log(1, "%s: no instance found\n", __FUNCTION__);
  return 0;
}

instanceEditMode AssimpSettingsContainer::getCurrentEditMode() {
  if (!mUndoStack.empty()) {
    return mUndoStack.top().ursEditMode;
  }
  if (!mRedoStack.empty()) {
    return mRedoStack.top().ursSavedEditMode;
  }
  /* fallback to edit mode */
  return instanceEditMode::move;
}

void AssimpSettingsContainer::applyNewInstance(std::shared_ptr<AssimpInstance> instance, int selectedInstanceId, int prevSelectedInstanceId) {
  Logger::log(1, "%s: add new instance\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::addInstance;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSavedEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSelectedInstance = selectedInstanceId;
  undoSettings.ursSavedSelectedInstance = prevSelectedInstanceId;

  AssimpInstanceSettings instSettings;
  instSettings.aisInstance = instance;;
  instSettings.aisInstanceSettings = instance->getInstanceSettings();

  undoSettings.ursInstanceSettings = instSettings;
  mUndoStack.emplace(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applyDeleteInstance(std::shared_ptr<AssimpInstance> instance, int selectedInstanceId, int prevSelectedInstanceId) {
  Logger::log(1, "%s: delete instance\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::deleteInstance;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSavedEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSelectedInstance = selectedInstanceId;
  undoSettings.ursSavedSelectedInstance = prevSelectedInstanceId;

  AssimpInstanceSettings instSettings;
  instSettings.aisDeletedInstance = instance;;

  undoSettings.ursInstanceSettings = instSettings;
  mUndoStack.push(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applyEditInstanceSettings(std::shared_ptr<AssimpInstance> instance, InstanceSettings newSettings, InstanceSettings oldSettings) {
  Logger::log(1, "%s: save instance settings\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::changeInstance;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSavedEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSelectedInstance = getSelectedInstanceCallbackFunction();
  undoSettings.ursSavedSelectedInstance = getSelectedInstanceCallbackFunction();

  AssimpInstanceSettings instSettings;
  instSettings.aisInstance = instance;;
  instSettings.aisInstanceSettings = newSettings;
  instSettings.aisSavedInstanceSettings = oldSettings;

  undoSettings.ursInstanceSettings = instSettings;
  mUndoStack.emplace(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applyNewMultiInstance(std::vector<std::shared_ptr<AssimpInstance>> instances, int selectedInstanceId, int prevSelectedInstanceId) {
  Logger::log(1, "%s: save multi instance\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::multiInstance;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSavedEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSelectedInstance = selectedInstanceId;
  undoSettings.ursSavedSelectedInstance = prevSelectedInstanceId;

  for (auto& instance : instances) {
    AssimpInstanceSettings instSettings;
    instSettings.aisInstance = instance;
    instSettings.aisSavedInstanceSettings = instance->getInstanceSettings();

    undoSettings.ursMultiInstanceSettings.amisMultiInstanceSettings.emplace_back(instSettings);
    undoSettings.ursMultiInstanceSettings.amisModelFileName = instance->getModel()->getModelFileNamePath();
  }

  mUndoStack.push(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applyChangeEditMode(instanceEditMode editMode, instanceEditMode savedEditMode) {
  Logger::log(1, "%s: save instance mode (new: %i, old: %i)\n", __FUNCTION__, editMode, savedEditMode);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::editMode;
  undoSettings.ursEditMode = editMode;
  undoSettings.ursSavedEditMode = savedEditMode;
  undoSettings.ursSelectedInstance = getSelectedInstanceCallbackFunction();

  mUndoStack.push(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applySelectInstance(int selectedInstanceId, int savedSelectedInstanceId) {
  Logger::log(1, "%s: select instance\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::selectInstance;
  undoSettings.ursSelectedInstance = selectedInstanceId;
  undoSettings.ursSavedSelectedInstance = savedSelectedInstanceId;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();

  mUndoStack.push(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applyLoadModel(std::shared_ptr<AssimpModel> model, int indexPos,
    std::shared_ptr<AssimpInstance> firstInstance,
    int selectedModelId, int prevSelectedModelId, int selectedInstanceId, int prevSelectedInstanceId) {
  Logger::log(1, "%s: add model\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::addModel;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSavedEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSelectedInstance = selectedInstanceId;
  undoSettings.ursSavedSelectedInstance = prevSelectedInstanceId;

  AssimpModelSettings modelSettings;
  modelSettings.amsModelFileName = model->getModelFileNamePath();
  modelSettings.amsModelPosInList = indexPos;
  modelSettings.amsSelectedModel = selectedModelId;
  modelSettings.amsSavedSelectedModel = prevSelectedModelId;
  modelSettings.amsModel = model;

  if (firstInstance) {
    modelSettings.amsInitialInstance = firstInstance;
  }

  undoSettings.ursModelSettings = modelSettings;
  mUndoStack.push(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applyDeleteModel(std::shared_ptr<AssimpModel> model, int indexPos, std::vector<std::shared_ptr<AssimpInstance>> instances,
      int selectedModelId, int prevSelectedModelId, int selectedInstanceId, int prevSelectedInstanceId) {
  Logger::log(1, "%s: delete model\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::deleteModel;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSavedEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSelectedInstance = selectedInstanceId;
  undoSettings.ursSavedSelectedInstance = prevSelectedInstanceId;

  AssimpModelSettings modelSettings;
  modelSettings.amsModelFileName = model->getModelFileNamePath();
  modelSettings.amsDeletedModel = model;
  modelSettings.amsModelPosInList = indexPos;
  modelSettings.amsSelectedModel = selectedModelId;
  modelSettings.amsSavedSelectedModel = prevSelectedModelId;

  if (!instances.empty()) {
    modelSettings.amsDeletedInstances = instances;
  }
  undoSettings.ursModelSettings = modelSettings;
  mUndoStack.push(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

void AssimpSettingsContainer::applyEditCameraSettings(std::shared_ptr<Camera> camera, CameraSettings newSettings, CameraSettings oldSettings) {
  Logger::log(1, "%s: save camera settings\n", __FUNCTION__);

  UndoRedoSettings undoSettings;
  undoSettings.ursObjectType = undoRedoObjectType::changeCamera;
  undoSettings.ursEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSavedEditMode = getInstanceEditModeCallbackFunction();
  undoSettings.ursSelectedInstance = getSelectedInstanceCallbackFunction();
  undoSettings.ursSavedSelectedInstance = getSelectedInstanceCallbackFunction();

  CameraSavedSettings camSettings;
  camSettings.ccsCamera = camera;
  camSettings.cssCameraSettings = newSettings;
  camSettings.cssSavedCameraSettings = oldSettings;

  undoSettings.ursCameraSetings = camSettings;
  mUndoStack.emplace(undoSettings);

  /* clear redo history on apply, makes no sense to keep */
  removeRedoStack();
}

int AssimpSettingsContainer::getUndoSize() {
  return mUndoStack.size();
}

int AssimpSettingsContainer::getRedoSize() {
  return mRedoStack.size();
}

void AssimpSettingsContainer::undo() {
  if (mUndoStack.empty()) {
    return;
  }

  UndoRedoSettings& undoSettings = mUndoStack.top();
  Logger::log(2, "%s: found undo for type %i\n", __FUNCTION__, undoSettings.ursObjectType);

  switch (undoSettings.ursObjectType) {
    case undoRedoObjectType::changeInstance:
      {
        if (std::shared_ptr<AssimpInstance> instance = undoSettings.ursInstanceSettings.aisInstance.lock()) {
          instance->setInstanceSettings(undoSettings.ursInstanceSettings.aisSavedInstanceSettings);
        }

        setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
        setSelectedInstanceCallbackFunction(undoSettings.ursSavedSelectedInstance);
      }
      break;
    case undoRedoObjectType::addInstance:
        if (std::shared_ptr<AssimpInstance> instance = undoSettings.ursInstanceSettings.aisInstance.lock()) {
          undoSettings.ursInstanceSettings.aisDeletedInstance = instance;
          instanceDeleteCallbackFunction(instance, false);
        }

        setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
        setSelectedInstanceCallbackFunction(undoSettings.ursSavedSelectedInstance);
     break;
    case undoRedoObjectType::deleteInstance:
      instanceAddExistingCallbackFunction(undoSettings.ursInstanceSettings.aisDeletedInstance,
                                          undoSettings.ursInstanceSettings.aisDeletedInstance->getInstanceIndexPosition(),
                                          undoSettings.ursInstanceSettings.aisDeletedInstance->getInstancePerModelIndexPosition());
        undoSettings.ursInstanceSettings.aisInstance = undoSettings.ursInstanceSettings.aisDeletedInstance;
        undoSettings.ursInstanceSettings.aisDeletedInstance.reset();

        setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
        setSelectedInstanceCallbackFunction(undoSettings.ursSavedSelectedInstance);
      break;
    case undoRedoObjectType::multiInstance:
      for (auto iter = undoSettings.ursMultiInstanceSettings.amisMultiInstanceSettings.begin(); iter != undoSettings.ursMultiInstanceSettings.amisMultiInstanceSettings.end(); ++iter) {
        if (std::shared_ptr<AssimpInstance> instance = (*iter).aisInstance.lock()) {
          (*iter).aisDeletedInstance = instance;
        }
      }
      for (auto iter = undoSettings.ursMultiInstanceSettings.amisMultiInstanceSettings.rbegin(); iter != undoSettings.ursMultiInstanceSettings.amisMultiInstanceSettings.rend(); ++iter) {
        if (std::shared_ptr<AssimpInstance> instance = (*iter).aisInstance.lock()) {
          instanceDeleteCallbackFunction(instance, false);
        }
      }
      setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
      setSelectedInstanceCallbackFunction(undoSettings.ursSavedSelectedInstance);
      break;
    case undoRedoObjectType::addModel:
      if (std::shared_ptr<AssimpInstance> instance = undoSettings.ursModelSettings.amsInitialInstance.lock()) {
        undoSettings.ursModelSettings.amsDeletedInstances.emplace_back(instance);
        undoSettings.ursModelSettings.amsInitialInstance.reset();
        instanceDeleteCallbackFunction(instance, false);
      } else {
        Logger::log(1, "%s: no initial instance\n", __FUNCTION__);
      }

      setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
      setSelectedInstanceCallbackFunction(undoSettings.ursSavedSelectedInstance);

      if (std::shared_ptr<AssimpModel> model = undoSettings.ursModelSettings.amsModel.lock()) {
        undoSettings.ursModelSettings.amsDeletedModel = model;
        undoSettings.ursModelSettings.amsModel.reset();
      } else {
        Logger::log(1, "%s error: could not find model for '%s'\n", __FUNCTION__, undoSettings.ursModelSettings.amsModelFileName.c_str());
      }

      setSelectedModelCallbackFunction(undoSettings.ursModelSettings.amsSavedSelectedModel);

      modelDeleteCallbackFunction(undoSettings.ursModelSettings.amsModelFileName, false);
      break;
    case undoRedoObjectType::deleteModel:
      modelAddExistingCallbackFunction(undoSettings.ursModelSettings.amsDeletedModel, undoSettings.ursModelSettings.amsModelPosInList);
      undoSettings.ursModelSettings.amsModel = undoSettings.ursModelSettings.amsDeletedModel;
      undoSettings.ursModelSettings.amsDeletedModel.reset();

      setSelectedModelCallbackFunction(undoSettings.ursModelSettings.amsSavedSelectedModel);

      /* restore initial instance */
      if (!undoSettings.ursModelSettings.amsDeletedInstances.empty()) {
        for (auto iter = undoSettings.ursModelSettings.amsDeletedInstances.begin(); iter != undoSettings.ursModelSettings.amsDeletedInstances.end(); ++iter) {
          instanceAddExistingCallbackFunction((*iter), (*iter)->getInstanceIndexPosition(), (*iter)->getInstancePerModelIndexPosition());
          undoSettings.ursModelSettings.amsInstances.emplace_back(*iter);
        }
      }
      undoSettings.ursModelSettings.amsDeletedInstances.clear();

      setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
      setSelectedInstanceCallbackFunction(undoSettings.ursSavedSelectedInstance);
      break;
    case undoRedoObjectType::editMode:
      setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
      break;
    case undoRedoObjectType::selectInstance:
      setSelectedInstanceCallbackFunction(undoSettings.ursSelectedInstance);
      break;
    case undoRedoObjectType::changeCamera:
    {
      if (std::shared_ptr<Camera> camera = undoSettings.ursCameraSetings.ccsCamera.lock()) {
        camera->setCameraSettings(undoSettings.ursCameraSetings.cssSavedCameraSettings);
        Logger::log(1, "%s: FOV is now %i\n", __FUNCTION__, undoSettings.ursCameraSetings.cssSavedCameraSettings.csFieldOfView);
      }

      setInstanceEditModeCallbackFunction(undoSettings.ursSavedEditMode);
      setSelectedInstanceCallbackFunction(undoSettings.ursSavedSelectedInstance);
    }
    break;
    default:
      Logger::log(1, "%s error: unknown undo type\n", __FUNCTION__);
      break;
  }

  if (!mUndoStack.empty()) {
    mRedoStack.emplace(mUndoStack.top());
    mUndoStack.pop();
  }
}

void AssimpSettingsContainer::redo() {
  if (mRedoStack.empty()) {
    return;
  }

  UndoRedoSettings& redoSettings = mRedoStack.top();
  Logger::log(2, "%s: found redo for type %i\n", __FUNCTION__, redoSettings.ursObjectType);

  switch (redoSettings.ursObjectType) {
    case undoRedoObjectType::changeInstance:
      {
        if (std::shared_ptr<AssimpInstance> instance = redoSettings.ursInstanceSettings.aisInstance.lock()) {
          instance->setInstanceSettings(redoSettings.ursInstanceSettings.aisInstanceSettings);
        }

        setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
        setSelectedInstanceCallbackFunction(redoSettings.ursSelectedInstance);
      }
      break;
    case undoRedoObjectType::addInstance:
      instanceAddExistingCallbackFunction(redoSettings.ursInstanceSettings.aisDeletedInstance,
                                          redoSettings.ursInstanceSettings.aisDeletedInstance->getInstanceIndexPosition(),
                                          redoSettings.ursInstanceSettings.aisDeletedInstance->getInstancePerModelIndexPosition());
        redoSettings.ursInstanceSettings.aisInstance = redoSettings.ursInstanceSettings.aisDeletedInstance;
        redoSettings.ursInstanceSettings.aisDeletedInstance.reset();

        setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
        setSelectedInstanceCallbackFunction(redoSettings.ursSelectedInstance);
      break;
    case undoRedoObjectType::deleteInstance:
        if (std::shared_ptr<AssimpInstance> instance = redoSettings.ursInstanceSettings.aisInstance.lock()) {
          redoSettings.ursInstanceSettings.aisDeletedInstance = instance;
          /* do not record this for undo */
          instanceDeleteCallbackFunction(instance, false);
        }

        setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
        setSelectedInstanceCallbackFunction(redoSettings.ursSelectedInstance);
      break;
    case undoRedoObjectType::addModel:
      modelAddExistingCallbackFunction(redoSettings.ursModelSettings.amsDeletedModel, redoSettings.ursModelSettings.amsModelPosInList);
      redoSettings.ursModelSettings.amsModel = redoSettings.ursModelSettings.amsDeletedModel;
      redoSettings.ursModelSettings.amsDeletedModel.reset();

      setSelectedModelCallbackFunction(redoSettings.ursModelSettings.amsSelectedModel);

      /* restore initial instance */
      if (redoSettings.ursModelSettings.amsDeletedInstances.size() == 1) {
        instanceAddExistingCallbackFunction(redoSettings.ursModelSettings.amsDeletedInstances.at(0),
                                            redoSettings.ursModelSettings.amsDeletedInstances.at(0)->getInstanceIndexPosition(),
                                            redoSettings.ursModelSettings.amsDeletedInstances.at(0)->getInstancePerModelIndexPosition());
        redoSettings.ursModelSettings.amsInitialInstance = redoSettings.ursModelSettings.amsDeletedInstances.at(0);
        redoSettings.ursModelSettings.amsDeletedInstances.clear();
      }

      setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
      setSelectedInstanceCallbackFunction(redoSettings.ursSelectedInstance);
      break;
    case undoRedoObjectType::deleteModel:
      if (!redoSettings.ursModelSettings.amsInstances.empty()) {
        for (auto iter = redoSettings.ursModelSettings.amsInstances.begin(); iter != redoSettings.ursModelSettings.amsInstances.end(); ++iter)  {
          if (std::shared_ptr<AssimpInstance> instance = (*iter).lock()) {
            redoSettings.ursModelSettings.amsDeletedInstances.emplace_back(instance);
          } else {
            Logger::log(1, "%s error: could not insert new instance in deleteModel\n", __FUNCTION__);
          }
        }
        for (auto iter = redoSettings.ursModelSettings.amsInstances.rbegin(); iter != redoSettings.ursModelSettings.amsInstances.rend(); ++iter)  {
          if (std::shared_ptr<AssimpInstance> instance = (*iter).lock()) {
            instanceDeleteCallbackFunction(instance, false);
          } else {
            Logger::log(1, "%s error: could not delete instance in deleteModel\n", __FUNCTION__);
          }
        }
      }
      redoSettings.ursModelSettings.amsInstances.clear();

      setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
      setSelectedInstanceCallbackFunction(redoSettings.ursSelectedInstance);

      if (std::shared_ptr<AssimpModel> model = redoSettings.ursModelSettings.amsModel.lock()) {
        redoSettings.ursModelSettings.amsDeletedModel = model;
        redoSettings.ursModelSettings.amsModel.reset();
      } else {
        Logger::log(1, "%s error: could not find model for '%s'\n", __FUNCTION__, redoSettings.ursModelSettings.amsModelFileName.c_str());
      }

      setSelectedModelCallbackFunction(redoSettings.ursModelSettings.amsSelectedModel);

      modelDeleteCallbackFunction(redoSettings.ursModelSettings.amsModelFileName, false);
      break;
    case undoRedoObjectType::multiInstance:
      {
        std::shared_ptr<AssimpModel> model = instanceGetModelCallbackFunction(redoSettings.ursMultiInstanceSettings.amisModelFileName);
        if (!model) {
          Logger::log(1, "%s error: model '%s' is no longer loaded, skipping redo\n", __FUNCTION__, redoSettings.ursMultiInstanceSettings.amisModelFileName.c_str());
          return;
        }

        for (auto& instSettings : redoSettings.ursMultiInstanceSettings.amisMultiInstanceSettings) {
          instanceAddExistingCallbackFunction(instSettings.aisDeletedInstance, instSettings.aisDeletedInstance->getInstanceIndexPosition(),
                                              instSettings.aisDeletedInstance->getInstancePerModelIndexPosition());
          instSettings.aisInstance = instSettings.aisDeletedInstance;
          instSettings.aisDeletedInstance.reset();
        }
        setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
        setSelectedInstanceCallbackFunction(redoSettings.ursSelectedInstance);
      }
      break;
    case undoRedoObjectType::editMode:
      setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
      break;
    case undoRedoObjectType::selectInstance:
      setSelectedInstanceCallbackFunction(redoSettings.ursSavedSelectedInstance);
      break;
    case undoRedoObjectType::changeCamera:
      {
        if (std::shared_ptr<Camera> camera = redoSettings.ursCameraSetings.ccsCamera.lock()) {
          camera->setCameraSettings(redoSettings.ursCameraSetings.cssCameraSettings);
          Logger::log(1, "%s: FOV is now %i\n", __FUNCTION__, redoSettings.ursCameraSetings.cssSavedCameraSettings.csFieldOfView);
        }

        setInstanceEditModeCallbackFunction(redoSettings.ursEditMode);
        setSelectedInstanceCallbackFunction(redoSettings.ursSelectedInstance);
      }
      break;
    default:
      Logger::log(1, "%s error: unknown redo type\n", __FUNCTION__);
      break;
  }

  if (!mRedoStack.empty()) {
    mUndoStack.emplace(mRedoStack.top());
    mRedoStack.pop();
  }
}
