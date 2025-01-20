#pragma once

#include <string>
#include <glm/glm.hpp>

struct LevelSettings {
  std::string lsLevelFilenamePath;
  std::string lsLevelFilename;

  glm::vec3 lsWorldPosition = glm::vec3(0.0f);
  glm::vec3 lsWorldRotation = glm::vec3(0.0f);
  float lsScale = 1.0f;
  bool lsSwapYZAxis = false;
};
