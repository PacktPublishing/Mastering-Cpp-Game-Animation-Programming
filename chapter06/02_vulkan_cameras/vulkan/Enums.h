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
  selectInstance,
  changeCamera,
  addCamera,
  deleteCamera
};

enum class cameraType {
  free = 0,
  firstPerson,
  thirdPerson,
  stationary,
  stationaryFollowing
};

enum class cameraProjection {
  perspective = 0,
  orthogonal
};
