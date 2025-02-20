#define _USE_MATH_DEFINES
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Camera.h"
#include "Logger.h"

void Camera::updateCamera(OGLRenderData& renderData, const float deltaTime) {
  if (deltaTime == 0.0f) {
    return;
  }

  float azimRad = glm::radians(renderData.rdViewAzimuth);
  float elevRad = glm::radians(renderData.rdViewElevation);

  float sinAzim = std::sin(azimRad);
  float cosAzim = std::cos(azimRad);
  float sinElev = std::sin(elevRad);
  float cosElev = std::cos(elevRad);

  /* update view direction */
  mViewDirection = glm::normalize(glm::vec3(
     sinAzim * cosElev, sinElev, -cosAzim * cosElev));

  /* calculate right and up direction */
  mRightDirection = glm::normalize(glm::cross(mViewDirection, mWorldUpVector));
  mUpDirection = glm::normalize(glm::cross(mRightDirection, mViewDirection));

  /* update camera position depending on desired movement */
  renderData.rdCameraWorldPosition +=
    renderData.rdMoveForward * deltaTime * mViewDirection +
      renderData.rdMoveRight * deltaTime * mRightDirection +
      renderData.rdMoveUp * deltaTime * mUpDirection;
}

void Camera::moveCameraTo(OGLRenderData& renderData, glm::vec3 position) {
  renderData.rdCameraWorldPosition = position;
  /* hard-code values for now, reversing from lookAt() matrix is too much work */
  renderData.rdViewAzimuth = 310.0f;
  renderData.rdViewElevation = -15.0f;
}

glm::mat4 Camera::getViewMatrix(OGLRenderData &renderData) {
  return glm::lookAt(renderData.rdCameraWorldPosition,
    renderData.rdCameraWorldPosition + mViewDirection, mUpDirection);
}
