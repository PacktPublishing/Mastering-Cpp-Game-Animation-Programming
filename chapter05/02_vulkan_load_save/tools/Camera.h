/* Simple camera object */
#pragma once

#include <glm/glm.hpp>

#include "VkRenderData.h"

class Camera {
  public:
    void updateCamera(VkRenderData& renderData, const float deltaTime);
    void moveCameraTo(VkRenderData& renderData, glm::vec3 position);

    glm::mat4 getViewMatrix(VkRenderData &renderData);

  private:
    glm::vec3 mViewDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mRightDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 mUpDirection = glm::vec3(0.0f, 0.0f, 0.0f);

    /* world up is positive Y */
    glm::vec3 mWorldUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
};
