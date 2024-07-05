#pragma once

enum class appMode {
  edit = 0,
  view
};

enum class instanceEditMode {
  move = 0,
  rotate,
  scale
};

enum class undoRedoObjectType {
  changeInstance = 0,
  addInstance,
  deleteInstance,
  multiInstance,
  addModel,
  deleteModel,
  editMode,
  selection
};
