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

enum class collisionChecks : uint8_t {
  none = 0,
  boundingBox,
  boundingSpheres
};

enum class collisionDebugDraw : uint8_t {
  none = 0,
  colliding,
  selected,
  all
};

enum class graphNodeType : uint8_t {
  none = 0,
  root,
  test,
  wait,
  randomWait,
  selector,
  sequence,
  instanceMovement,
  event,
  action,
  debugLog,
  faceAnim,
  headAmin,
  randomNavigation,
  NUM
};

inline graphNodeType& operator++(graphNodeType& nodeType) {
  using T = std::underlying_type_t <graphNodeType>;
  int n = static_cast<T>(nodeType);
  ++n;
  nodeType = static_cast<graphNodeType>(n);
  return nodeType;
}

inline graphNodeType operator++(graphNodeType& nodeType, int) {
  graphNodeType copy = nodeType;
  ++nodeType;
  return copy;
}

inline graphNodeType operator+(graphNodeType nodeType, int num) {
  using T = std::underlying_type_t <graphNodeType>;
  int n = static_cast<T>(nodeType);
  n += num;
  nodeType = static_cast<graphNodeType>(n);
  return nodeType;
}

enum class instanceUpdateType : uint8_t {
  none = 0,
  moveState,
  moveDirection,
  speed,
  rotation,
  position,
  faceAnimIndex,
  faceAnimWeight,
  headAnim,
  navigation
};

enum class nodeEvent : uint8_t {
  none = 0,
  instanceToInstanceCollision,
  instanceToEdgeCollision,
  interaction,
  instanceToLevelCollision,
  navTargetReached,
  NUM
};

enum class interactionDebugDraw : uint8_t {
  none = 0,
  distance,
  facingTowardsUs,
  nearestCandidate
};

enum class faceAnimation : uint8_t {
  none = 0,
  angry,
  worried,
  surprised,
  happy
};

enum class headMoveDirection : uint8_t {
  left = 0,
  right,
  up,
  down,
  NUM
};

/* preMorning and postEvening are for the position changes between sun and moon */
enum class timeOfDay : uint8_t {
  midnight, // 00:00
  preMorning, // 05:59
  morning, // 06:00
  noon, // 12:00
  evening, // 18:00
  postEvening, // 18:01
  preMidnight, // 23:59, for rollover
  fullLight, // 24:00
  NUM
};

inline timeOfDay operator+(timeOfDay A, int num) {
  using T = std::underlying_type_t <timeOfDay>;
  int n = static_cast<T>(A);
  n += num;
  A = static_cast<timeOfDay>(n);
  return A;
}
