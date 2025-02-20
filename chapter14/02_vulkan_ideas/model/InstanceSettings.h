/* model specific settings */
#pragma once

#include <string>
#include <vector>
#include <memory>

#include <glm/glm.hpp>

#include "Enums.h"

struct MeshTriangle;

struct InstanceSettings {
  std::string isModelFile;

  glm::vec3 isWorldPosition = glm::vec3(0.0f);
  glm::vec3 isWorldRotation = glm::vec3(0.0f);
  float isScale = 1.0f;
  bool isSwapYZAxis = false;

  unsigned int isFirstAnimClipNr = 0;
  unsigned int isSecondAnimClipNr = 0;
  float isFirstClipAnimPlayTimePos = 0.0f;
  float isSecondClipAnimPlayTimePos = 0.0f;
  float isAnimSpeedFactor = 1.0f;
  float isAnimBlendFactor = 0.0f;

  float isHeadLeftRightMove = 0.0f;
  float isHeadUpDownMove = 0.0f;

  int isInstanceIndexPosition = -1;
  int isInstancePerModelIndexPosition = -1;

  bool isNoMovement = false;
  glm::vec3 isAccel = glm::vec3(0.0f);
  bool isMoveKeyPressed = false;
  glm::vec3 isSpeed = glm::vec3(0.0f);
  moveDirection isMoveDirection = moveDirection::none;
  moveState isMoveState = moveState::idle;

  std::string isNodeTreeName;

  faceAnimation isFaceAnimType = faceAnimation::none;
  float isFaceAnimWeight = 0.0f;

  bool isInstanceOnGround = false;
  std::vector<MeshTriangle> isCollidingTriangles{};
  int isCurrentGroundTriangleIndex = -1;
  std::vector<int> isNeighborGroundTriangles{};

  bool isNavigationEnabled = false;
  int isPathTargetInstance = -1;
  int isPathStartTriangleIndex = -1;
  int isPathTargetTriangleIndex = -1;
  std::vector<int> isPathToTarget{};
};

/* temporary struct to catch the camera names from the save file */
struct ExtendedInstanceSettings : InstanceSettings {
  std::vector<std::string> eisCameraNames{};
};
