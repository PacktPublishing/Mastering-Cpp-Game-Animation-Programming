#pragma once

#include <string>
#include <stack>
#include <memory>

#include <glm/glm.hpp>

#include "Enums.h"
#include "Callbacks.h"

#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "InstanceSettings.h"
#include "ModelAndInstanceData.h"

struct AssimpInstanceSettings {
  std::weak_ptr<AssimpInstance> aisInstance{};
  std::shared_ptr<AssimpInstance> aisDeletedInstance{};
  InstanceSettings aisInstanceSettings{};
  InstanceSettings aisSavedInstanceSettings{};
};

struct AssimpMultiInstanceSettings {
  std::string amisModelFileName;
  std::vector<AssimpInstanceSettings> amisMultiInstanceSettings{};
};

struct AssimpModelSettings {
  std::string amsModelFileName;
  int amsSelectedModel;
  int amsSavedSelectedModel;
  int amsModelPosInList;
  std::weak_ptr<AssimpModel> amsModel;
  std::shared_ptr<AssimpModel> amsDeletedModel;
  std::weak_ptr<AssimpInstance> amsInitialInstance;
  std::vector<std::weak_ptr<AssimpInstance>> amsInstances{};
  std::vector<std::shared_ptr<AssimpInstance>> amsDeletedInstances{};
};

struct UndoRedoSettings {
  undoRedoObjectType ursObjectType;
  instanceEditMode ursEditMode;
  instanceEditMode ursSavedEditMode;
  int ursSelectedInstance;
  int ursSavedSelectedInstance;
  AssimpInstanceSettings ursInstanceSettings;
  AssimpMultiInstanceSettings ursMultiInstanceSettings;
  AssimpModelSettings ursModelSettings;
};

class AssimpSettingsContainer {
  public:
    AssimpSettingsContainer(std::shared_ptr<AssimpInstance> nullInstance);

    int getCurrentInstance();
    instanceEditMode getCurrentEditMode();

    void applyNewInstance(std::shared_ptr<AssimpInstance> instance, int selectedInstanceId, int prevSelectedInstanceId);
    void applyDeleteInstance(std::shared_ptr<AssimpInstance> instance, int selectedInstanceId, int prevSelectedInstanceId);
    void applyNewMultiInstance(std::vector< std::shared_ptr<AssimpInstance>> instances, int selectedInstanceId, int prevSelectedInstanceId);
    void applyEditInstanceSettings(std::shared_ptr<AssimpInstance> instance, InstanceSettings newSettings, InstanceSettings oldSettings);

    void applyLoadModel(std::shared_ptr<AssimpModel> model, int indexPos, std::shared_ptr<AssimpInstance> firstInstance,
      int selectedModelId, int prevSelectedModelId, int selectedInstanceId, int prevSelectedInstanceId);
    void applyDeleteModel(std::shared_ptr<AssimpModel> model, int indexPos, std::vector<std::shared_ptr<AssimpInstance>> instances,
      int selectedModelId, int prevSelectedModelId, int selectedInstanceId, int prevSelectedInstanceId);

    void applySelectInstance(int selectedInstanceId, int savedSelectedInstanceId);
    void applyChangeEditMode(instanceEditMode editMode, instanceEditMode savedEditMode);

    void undo();
    void redo();

    getSelectedInstanceCallback getSelectedInstanceCallbackFunction;
    setSelectedInstanceCallback setSelectedInstanceCallbackFunction;
    getInstanceEditModeCallback getInstanceEditModeCallbackFunction;
    setInstanceEditModeCallback setInstanceEditModeCallbackFunction;

    instanceGetModelCallback instanceGetModelCallbackFunction;
    instanceAddCallback instanceAddCallbackFunction;
    instanceAddExistingCallback instanceAddExistingCallbackFunction;
    instanceDeleteCallback instanceDeleteCallbackFunction;

    getSelectedModelCallback getSelectedModelCallbackFunction;
    setSelectedModelCallback setSelectedModelCallbackFunction;

    modelDeleteCallback modelDeleteCallbackFunction;
    modelAddCallback modelAddCallbackFunction;
    modelAddExistingCallback modelAddExistingCallbackFunction;

    int getUndoSize();
    int getRedoSize();

    void removeStacks();

  private:
    std::weak_ptr<AssimpInstance> mNullInstance;

    void removeRedoStack();

    std::stack<UndoRedoSettings> mUndoStack{};
    std::stack<UndoRedoSettings> mRedoStack{};
};
