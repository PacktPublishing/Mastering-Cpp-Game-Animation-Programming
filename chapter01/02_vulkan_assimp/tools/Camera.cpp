#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

void Camera::updateCamera(VkRenderData& renderData, const float deltaTime) {
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

glm::mat4 Camera::getViewMatrix(VkRenderData &renderData) {
  return glm::lookAt(renderData.rdCameraWorldPosition,
    renderData.rdCameraWorldPosition + mViewDirection, mUpDirection);
}
