#pragma once

#include <string>
#include <tuple>
#include <array>
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

  float msForwardSpeedFactor = 4.0f;

  std::map<headMoveDirection, int> msHeadMoveClipMappings{};

  /* first in array is left, second is right foot */
  /* first in pair is effeltor node, second is chain root node */
  std::array<std::pair<int, int>, 2> msFootIKChainPair{};
  std::array<std::vector<int>, 2> msFootIKChainNodes{};

  bool msUseAsNavigationTarget = false;

  bool msPreviewMode = false;
};
