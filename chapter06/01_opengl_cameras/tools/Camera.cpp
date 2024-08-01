#define _USE_MATH_DEFINES
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "Camera.h"
#include "InstanceSettings.h"
#include "Logger.h"

std::string Camera::getName() {
  return mCamSettings.csCamName;
}

void Camera::setName(std::string name) {
  mCamSettings.csCamName = name;
}

void Camera::updateCamera(OGLRenderData& renderData, float deltaTime) {
  if (deltaTime == 0.0f) {
    return;
  }

  /* no camera movement on stationary cam */
  if (mCamSettings.csCamType == cameraType::stationary) {
    return;
  }

  /* default handling is free camera if nothing has been locked  */
  if (!mCamSettings.csInstanceToFollow.lock()) {
    updateCameraView(renderData, deltaTime);
    updateCameraPosition(renderData, deltaTime);
    return;
  }

  /* follow the locked object depending on the camera type */
  switch (mCamSettings.csCamType) {
    case cameraType::firstPerson:
      {
        mCamSettings.csWorldPosition = mFirstPersonBoneMatrix[3];

        if (mCamSettings.csFirstPersonLockView) {
          /* get elevation */
          glm::vec3 elevationVector = mFirstPersonBoneMatrix * glm::vec4(mWorldUpVector, 0.0f);
          mCamSettings.csViewElevation = -glm::degrees(glm::acos(glm::dot(glm::normalize(elevationVector), mWorldUpVector)));

          glm::vec3 azimuthVector = mFirstPersonBoneMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
          float rotateAngle = glm::degrees(glm::acos(glm::dot(glm::normalize(glm::vec3(azimuthVector.x, 0.0f, azimuthVector.z)), glm::vec3(0.0f, 0.0f, -1.0f))));
          /* support full 360 degree for Azimuth */
          if (azimuthVector.x < 0.0f) {
            rotateAngle = 360.0f - rotateAngle;
          }
          mCamSettings.csViewAzimuth = rotateAngle;
        }

        updateCameraView(renderData, deltaTime);
      }
      break;
    case cameraType::thirdPerson:
      {
        std::shared_ptr<AssimpInstance> instance = mCamSettings.csInstanceToFollow.lock();
        InstanceSettings instSettings = instance->getInstanceSettings();

        float rotationAngle = 180.0f - instSettings.isWorldRotation.y;
        mCamSettings.csViewAzimuth = rotationAngle;

        glm::vec3 offset = glm::vec3(-glm::sin(glm::radians(rotationAngle)), 1.0f, glm::cos(glm::radians(rotationAngle))) * mCamSettings.csThirdPersonDistance;
        offset.y += mCamSettings.csThirdPersonHeightOffset;
        mCamSettings.csWorldPosition = instSettings.isWorldPosition + offset;

        glm::vec3 viewDirection = instSettings.isWorldPosition - mCamSettings.csWorldPosition;
        mCamSettings.csViewElevation = (90.0f - glm::degrees(glm::acos(glm::dot(glm::normalize(viewDirection), mWorldUpVector)))) / 2.0f;

        updateCameraView(renderData, deltaTime);
      }
      break;
    case cameraType::stationaryFollowing:
      {
        std::shared_ptr<AssimpInstance> instance = mCamSettings.csInstanceToFollow.lock();
        glm::vec3 viewDirection = instance->getWorldPosition() - mCamSettings.csWorldPosition;

        mCamSettings.csViewElevation = 90.0f - glm::degrees(glm::acos(glm::dot(glm::normalize(viewDirection), mWorldUpVector)));

        /* map to 'y = 0' to avoid elevation angle taking over for the largest angle */
        float rotateAngle = glm::degrees(glm::acos(glm::dot(glm::normalize(glm::vec3(viewDirection.x, 0.0f, viewDirection.z)), glm::vec3(0.0f, 0.0f, -1.0f))));
        /* support full 360 degree for Azimuth */
        if (viewDirection.x < 0.0f) {
          rotateAngle = 360.0f - rotateAngle;
        }
        mCamSettings.csViewAzimuth = rotateAngle;

        updateCameraView(renderData, deltaTime);
      }
      break;
    default:
      Logger::log(1, "%s error: unknown camera type\n", __FUNCTION__);
      break;
  }
}

void Camera::updateCameraView(OGLRenderData& renderData, const float deltaTime) {
  float azimRad = glm::radians(mCamSettings.csViewAzimuth);
  float elevRad = glm::radians(mCamSettings.csViewElevation);

  float sinAzim = glm::sin(azimRad);
  float cosAzim = glm::cos(azimRad);
  float sinElev = glm::sin(elevRad);
  float cosElev = glm::cos(elevRad);

  /* update view direction */
  mViewDirection = glm::normalize(glm::vec3(
     sinAzim * cosElev, sinElev, -cosAzim * cosElev));

  /* calculate right and up direction */
  mRightDirection = glm::normalize(glm::cross(mViewDirection, mWorldUpVector));
  mUpDirection = glm::normalize(glm::cross(mRightDirection, mViewDirection));
}

void Camera::updateCameraPosition(OGLRenderData& renderData, const float deltaTime){
  /* update camera position depending on desired movement */
  mCamSettings.csWorldPosition +=
    renderData.rdMoveForward * deltaTime * mViewDirection
    + renderData.rdMoveRight * deltaTime * mRightDirection
    + renderData.rdMoveUp * deltaTime * mUpDirection;
}


void Camera::moveCameraTo(glm::vec3 position) {
  mCamSettings.csWorldPosition = position;
  /* hard-code values for now, reversing from lookAt() matrix is too much work */
  mCamSettings.csViewAzimuth = 310.0f;
  mCamSettings.csViewElevation = -15.0f;
}

glm::vec3 Camera::getWorldPosition() {
  return mCamSettings.csWorldPosition;
}

void Camera::setWorldPosition(glm::vec3 position) {
  mCamSettings.csWorldPosition = position;
}

float Camera::getAzimuth() {
  return mCamSettings.csViewAzimuth;
}

void Camera::setAzimuth(float azimuth) {
  mCamSettings.csViewAzimuth = azimuth;
}

float Camera::getElevation() {
  return mCamSettings.csViewElevation;
}

void Camera::setElevation(float elevation) {
  mCamSettings.csViewElevation = elevation;
}

float Camera::getFOV() {
  return mCamSettings.csFieldOfView;
}

void Camera::setFOV(float fieldOfView) {
  mCamSettings.csFieldOfView = fieldOfView;
}

float Camera::getOrthoScale() {
  return mCamSettings.csOrthoScale;
}

void Camera::setOrthoScale(float scale) {
  mCamSettings.csOrthoScale = scale;
}

glm::mat4 Camera::getViewMatrix() {
  return glm::lookAt(mCamSettings.csWorldPosition, mCamSettings.csWorldPosition + mViewDirection, mUpDirection);
}

cameraType Camera::getCameraType() {
  return mCamSettings.csCamType;
}

void Camera::setCameraType(cameraType type) {
  mCamSettings.csCamType = type;
}

cameraProjection Camera::getCameraProjection() {
  return mCamSettings.csCamProjection;
}

void Camera::setCameraProjection(cameraProjection proj) {
  mCamSettings.csCamProjection = proj;
}

CameraSettings Camera::getCameraSettings() {
  return mCamSettings;
}

void Camera::setCameraSettings(CameraSettings settings) {
  mCamSettings = settings;
}

std::shared_ptr<AssimpInstance> Camera::getInstanceToFollow() {
  if (std::shared_ptr<AssimpInstance> instance = mCamSettings.csInstanceToFollow.lock()) {
    return instance;
  }
  return nullptr;
}

void Camera::setInstanceToFollow(std::shared_ptr<AssimpInstance> instance) {
  if (instance) {
    mCamSettings.csInstanceToFollow = instance;
    mFirstPersonBoneNames = instance->getModel()->getBoneNameList();
  }
}

void Camera::clearInstanceToFollow() {
  mCamSettings.csInstanceToFollow.reset();
  mFirstPersonBoneNames.clear();
}

std::vector<std::string> Camera::getBoneNames() {
  return mFirstPersonBoneNames;
}

void Camera::setBoneMatrix(glm::mat4 matrix) {
  mFirstPersonBoneMatrix = matrix;
}
