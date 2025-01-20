#pragma once

#include <string>
#include <tuple>
#include <map>
#include <set>

#include "Enums.h"

struct IdleWalkRunBlending {
  int iwrbIdleClipNr = 0;
  float iwrbIdleClipSpeed = 1.0f;

  int iwrbWalkClipNr = 0;
  float iwrbWalkClipSpeed = 1.0f;

  int iwrbRunClipNr = 0;
  float iwrbRunClipSpeed = 1.0f;
};

struct ActionAnimation {
  int aaClipNr = 0;
  float aaClipSpeed = 1.0f;
};

struct ModelSettings {
  std::string msModelFilenamePath;
  std::string msModelFilename;

  std::map<moveState, ActionAnimation> msActionClipMappings{};
  std::map<moveDirection, IdleWalkRunBlending> msIWRBlendings{};
  std::set<std::pair<moveState, moveState>> msAllowedStateOrder{};

  std::vector<glm::vec4> msBoundingSphereAdjustments{};

  std::map<headMoveDirection, int> msHeadMoveClipMappings{};
};
