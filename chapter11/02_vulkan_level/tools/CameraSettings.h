/* per-camera settings */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

#include "Enums.h"

class AssimpInstance;

struct CameraSettings{
  std::string csCamName = "Camera";

  glm::vec3 csWorldPosition = glm::vec3(0.0f);
  float csViewAzimuth = 0.0f;
  float csViewElevation = 0.0f;

  int csFieldOfView = 90;
  float csOrthoScale = 20.0f;

  bool csFirstPersonLockView = true;
  int csFirstPersonBoneToFollow = 0;
  glm::vec3 csFirstPersonOffsets = glm::vec3(0.0f);

  float csThirdPersonDistance = 5.0f;
  float csThirdPersonHeightOffset = 0.0f;

  float csFollowCamHeightOffset = 2.0f;

  cameraType csCamType = cameraType::free;
  cameraProjection csCamProjection = cameraProjection::perspective;

  /* pointer is required here for undo/redo to work */
  std::weak_ptr<AssimpInstance> csInstanceToFollow;
};
