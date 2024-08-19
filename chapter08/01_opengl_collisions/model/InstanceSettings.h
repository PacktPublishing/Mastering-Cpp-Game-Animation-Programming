/* model specific settings */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "Enums.h"

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

  int isInstanceIndexPosition = -1;
  int isInstancePerModelIndexPosition = -1;

  bool rdNoMovement = false;
  glm::vec3 isAccel = glm::vec3(0.0f);
  bool isMoveKeyPressed = false;
  glm::vec3 isSpeed = glm::vec3(0.0f);
  moveDirection isMoveDirection = moveDirection::none;
  moveState isMoveState = moveState::idle;

  glm::vec3 isAutoRunSpeed = glm::vec3(0.5f + (std::rand()%200) / 100.0f, 0.0f, 0.0f);
};

/* temporary struct to catch the camera names from the save file */
struct ExtendedInstanceSettings : InstanceSettings {
  std::vector<std::string> eisCameraNames;
};
