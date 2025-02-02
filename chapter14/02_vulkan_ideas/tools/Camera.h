/* Simple camera object */
#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

#include "AssimpInstance.h"

#include "VkRenderData.h"
#include "CameraSettings.h"

class Camera {
  public:
    std::string getName();
    void setName(std::string name);

    void updateCamera(VkRenderData& renderData, const float deltaTime);
    void moveCameraTo(glm::vec3 position);

    glm::vec3 getWorldPosition();
    void setWorldPosition(glm::vec3 position);
    float getAzimuth();
    void setAzimuth(float azimuth);
    float getElevation();
    void setElevation(float elevation);

    glm::mat4 getViewMatrix();

    float getFOV();
    void setFOV(float fieldOfView);

    /* glm::ortho lets us scale the resulting view by scaling all values */
    float getOrthoScale();
    void setOrthoScale(float scale);

    cameraType getCameraType();
    void setCameraType(cameraType type);

    cameraProjection getCameraProjection();
    void setCameraProjection(cameraProjection proj);

    std::shared_ptr<AssimpInstance> getInstanceToFollow();
    void setInstanceToFollow(std::shared_ptr<AssimpInstance> instance);
    void clearInstanceToFollow();
    std::vector<std::string> getBoneNames();
    void setBoneMatrix(glm::mat4 matrix);

    CameraSettings getCameraSettings();
    void setCameraSettings(CameraSettings settings);

  private:
    void updateCameraView(VkRenderData& renderData, const float deltaTime);
    void updateCameraPosition(VkRenderData& renderData, const float deltaTime);

    CameraSettings mCamSettings{};

    glm::vec3 mViewDirection = glm::vec3(0.0f);
    glm::vec3 mRightDirection = glm::vec3(0.0f);
    glm::vec3 mUpDirection = glm::vec3(0.0f);

    /* world up is positive Y */
    const glm::vec3 mWorldUpVector = glm::vec3(0.0f, 1.0f, 0.0f);
    /* vector pointing to the z axis to have an Azimuth reference */
    const glm::vec3 mSideVector = glm::vec3(0.0f, 0.0f, 1.0f);

    glm::mat4 mFirstPersonBoneMatrix = glm::mat4(1.0f);
    std::vector<std::string> mFirstPersonBoneNames{};
};
