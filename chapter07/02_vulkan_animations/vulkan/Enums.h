#pragma once
#include <cstdint>

enum class appMode : uint8_t {
  edit = 0,
  view
};

inline appMode operator++ (appMode const& m) {
  return static_cast<appMode>((static_cast<int>(m) + 1) % 2);
}

inline appMode operator-- (appMode const& m) {
  return static_cast<appMode>(((static_cast<int>(m) - 1) % 2 + 2) % 2);
}

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

enum class moveState : uint8_t {
  idle = 0,
  walk,
  run,
  hop,
  jump,
  punch,
  pick,
  roll,
  kick,
  interact,
  wave,
  NUM
};

enum class moveDirection : uint8_t {
  none = 0x00,
  forward = 0x01,
  back = 0x02,
  right = 0x04,
  left = 0x08,
  any = 0xff
};

inline moveDirection operator | (moveDirection lhs, moveDirection rhs) {
  using T = std::underlying_type_t <moveDirection>;
  return static_cast<moveDirection>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline moveDirection& operator |= (moveDirection& lhs, moveDirection rhs) {
  lhs = lhs | rhs;
  return lhs;
}

inline moveDirection operator & (moveDirection lhs, moveDirection rhs) {
  using T = std::underlying_type_t <moveDirection>;
  return static_cast<moveDirection>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

inline moveDirection& operator &= (moveDirection& lhs, moveDirection rhs) {
  lhs = lhs & rhs;
  return lhs;
}

enum class animationState : uint8_t {
  playIdleWalkRun = 0,
  transitionFromIdleWalkRun,
  transitionToAction,
  playActionAnim,
  transitionToIdleWalkRun
};
