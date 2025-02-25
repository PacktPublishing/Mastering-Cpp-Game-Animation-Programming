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
  selectInstance,
  changeCamera,
  addCamera,
  deleteCamera
};

enum class cameraType : uint8_t {
  free = 0,
  firstPerson,
  thirdPerson,
  stationary,
  stationaryFollowing
};

enum class cameraProjection : uint8_t {
  perspective = 0,
  orthogonal
};
