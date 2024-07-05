/* model specific settings */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

struct InstanceSettings {
  std::string isModelFile;

  glm::vec3 isWorldPosition = glm::vec3(0.0f);
  glm::vec3 isWorldRotation = glm::vec3(0.0f);
  float isScale = 1.0f;
  bool isSwapYZAxis = false;

  unsigned int isAnimClipNr = 0;
  float isAnimPlayTimePos = 0.0f;
  float isAnimSpeedFactor = 1.0f;

  int isInstanceIndexPosition = -1;
  int isInstancePerModelIndexPosition = -1;
};

/* temporary struct to catch the camera names from the save file */
struct ExtendedInstanceSettings : InstanceSettings {
  std::vector<std::string> eisCameraNames;
};
