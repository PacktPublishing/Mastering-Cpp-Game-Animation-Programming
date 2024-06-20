#pragma once

#include <stack>
#include <memory>

#include "AssimpInstance.h"
#include "InstanceSettings.h"
#include "ModelAndInstanceData.h"

struct AssimpInstanceSettings {
  std::weak_ptr<AssimpInstance> aisInstance;
  InstanceSettings aisInstanceSettings{};
  InstanceSettings aisSavedInstanceSettings{};
};

class AssimpSettingsContainer {
  public:
    AssimpSettingsContainer(std::shared_ptr<AssimpInstance> nullInstance);

    std::shared_ptr<AssimpInstance> getCurrentInstance();

    void apply(std::shared_ptr<AssimpInstance> instance, InstanceSettings newSettings, InstanceSettings oldSettings);
    void undo();
    void redo();

    int getUndoSize();
    int getRedoSize();

    void removeStacks();

  private:
    std::weak_ptr<AssimpInstance> mNullInstance;

    std::stack<AssimpInstanceSettings> mUndoStack{};
    std::stack<AssimpInstanceSettings> mRedoStack{};
};
