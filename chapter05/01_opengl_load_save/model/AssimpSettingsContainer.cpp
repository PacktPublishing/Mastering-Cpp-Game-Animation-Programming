#include "AssimpSettingsContainer.h"
#include "Logger.h"

AssimpSettingsContainer::AssimpSettingsContainer(std::shared_ptr<AssimpInstance> nullInstance) {
  mNullInstance = nullInstance;
}

void AssimpSettingsContainer::removeStacks() {
  /* kill undo and redo, i.e., when loading a config */
  std::stack<AssimpInstanceSettings> emptyUndoStack = std::stack<AssimpInstanceSettings>();
  mUndoStack.swap(emptyUndoStack);

  std::stack<AssimpInstanceSettings> emptyRedoStack = std::stack<AssimpInstanceSettings>();
  mRedoStack.swap(emptyRedoStack);
}

std::shared_ptr<AssimpInstance> AssimpSettingsContainer::getCurrentInstance() {
  if (!mUndoStack.empty() && mUndoStack.top().aisInstance.lock()) {
    return mUndoStack.top().aisInstance.lock();
  }
  if (!mRedoStack.empty() && mRedoStack.top().aisInstance.lock()) {
    return mRedoStack.top().aisInstance.lock();
  }
  return mNullInstance.lock();
}

void AssimpSettingsContainer::apply(std::shared_ptr<AssimpInstance> instance, InstanceSettings newSettings, InstanceSettings oldSettings) {
  AssimpInstanceSettings settings;
  settings.aisInstance = instance;
  settings.aisInstanceSettings = newSettings;
  settings.aisSavedInstanceSettings = oldSettings;
  mUndoStack.emplace(settings);

  /* clear redo history on apply, makes no sense to keep */
  std::stack<AssimpInstanceSettings> emptyStack = std::stack<AssimpInstanceSettings>();
  mRedoStack.swap(emptyStack);
}

void AssimpSettingsContainer::undo() {
  /* cleanup in case instance was deleted */
  while (!mUndoStack.empty() && !mUndoStack.top().aisInstance.lock())  {
    Logger::log(1, "%s error: instance on undo %i missing, removing\n", __FUNCTION__, mUndoStack.size());
    mUndoStack.pop();
  }

  if (mUndoStack.empty()) {
    return;
  }

  AssimpInstanceSettings settings = mUndoStack.top();
  if (std::shared_ptr<AssimpInstance> instance = settings.aisInstance.lock()) {
    instance->setInstanceSettings(settings.aisSavedInstanceSettings);
  }

  if (!mUndoStack.empty()) {
    mRedoStack.emplace(mUndoStack.top());
    mUndoStack.pop();
  }
}

void AssimpSettingsContainer::redo() {
  /* cleanup in case instance was deleted */
  while (!mRedoStack.empty() && !mRedoStack.top().aisInstance.lock())  {
    Logger::log(1, "%s error: instance on redo %i missing, removing\n", __FUNCTION__, mRedoStack.size());
    mRedoStack.pop();
  }

  if (mRedoStack.empty()) {
    return;
  }

  AssimpInstanceSettings settings = mRedoStack.top();
  if (std::shared_ptr<AssimpInstance> instance = settings.aisInstance.lock()) {
    instance->setInstanceSettings(settings.aisInstanceSettings);
  }

  if (!mRedoStack.empty()) {
    mUndoStack.emplace(mRedoStack.top());
    mRedoStack.pop();
  }
}

int AssimpSettingsContainer::getUndoSize() {
  return mUndoStack.size();
}

int AssimpSettingsContainer::getRedoSize() {
  return mRedoStack.size();
}
