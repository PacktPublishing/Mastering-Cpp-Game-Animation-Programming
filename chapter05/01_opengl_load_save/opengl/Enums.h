#pragma once
#include <cstdint>

enum class appMode : uint8_t {
  edit = 0,
  view
};

enum class instanceEditMode : uint8_t {
  move = 0,
  rotate,
  scale
};

enum class undoRedoObjectType : uint8_t {
  changeInstance = 0,
  addInstance,
  deleteInstance,
  multiInstance,
  addModel,
  deleteModel,
  editMode,
  selection
};
