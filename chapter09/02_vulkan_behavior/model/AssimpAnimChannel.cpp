#include "AssimpAnimChannel.h"

#include "Logger.h"

void AssimpAnimChannel::loadChannelData(aiNodeAnim* nodeAnim, float maxClipDuration) {
  mNodeName = nodeAnim->mNodeName.C_Str();
  mNumTranslations = nodeAnim->mNumPositionKeys;
  mNumRotations = nodeAnim->mNumRotationKeys;
  mNumScalings = nodeAnim->mNumScalingKeys;
  mPreState = nodeAnim->mPreState;
  mPostState = nodeAnim->mPostState;

  mMinTranslateTime = static_cast<float>(nodeAnim->mPositionKeys[0].mTime);
  mMaxTranslateTime = static_cast<float>(nodeAnim->mPositionKeys[mNumTranslations - 1].mTime);

  Logger::log(1, "%s: - loading animation channel for node '%s', with %i translation keys, %i rotation keys, %i scaling keys (preState %i, postState %i, keys %f to %f)\n",
              __FUNCTION__, mNodeName.c_str(), mNumTranslations, mNumRotations, mNumScalings, mPreState, mPostState, mMinTranslateTime, mMaxTranslateTime);

  float translateScaleFactor = maxClipDuration / mMaxTranslateTime;
  mTranslateTimeScaleFactor = maxClipDuration / static_cast<float>(LOOKUP_TABLE_WIDTH);
  mInvTranslateTimeScaleFactor = 1.0f / mTranslateTimeScaleFactor;

  int timeIndex = 0;
  for (int i = 0; i < LOOKUP_TABLE_WIDTH; ++i) {
    glm::vec4 currentTranslate = glm::vec4(nodeAnim->mPositionKeys[timeIndex].mValue.x, nodeAnim->mPositionKeys[timeIndex].mValue.y, nodeAnim->mPositionKeys[timeIndex].mValue.z, 1.0f);
    glm::vec4 nextTranslate = glm::vec4(nodeAnim->mPositionKeys[timeIndex + 1].mValue.x, nodeAnim->mPositionKeys[timeIndex + 1].mValue.y, nodeAnim->mPositionKeys[timeIndex + 1].mValue.z, 1.0f);

    float currentKey = static_cast<float>(nodeAnim->mPositionKeys[timeIndex].mTime);
    float nextKey = static_cast<float>(nodeAnim->mPositionKeys[timeIndex + 1].mTime);
    float currentTime = i * mTranslateTimeScaleFactor / translateScaleFactor;

    mTranslations.emplace_back(glm::mix(currentTranslate, nextTranslate, (currentTime - currentKey) / (nextKey - currentKey)));

    if (currentTime > nextKey) {
      if (timeIndex < mNumTranslations - 1) {
        ++timeIndex;
      }
    }
  }

  mMinScaleTime = static_cast<float>(nodeAnim->mScalingKeys[0].mTime);
  mMaxScaleTime = static_cast<float>(nodeAnim->mScalingKeys[mNumScalings - 1].mTime);
  float scalingScaleFactor = maxClipDuration / mMaxScaleTime;
  mScaleTimeScaleFactor = maxClipDuration / static_cast<float>(LOOKUP_TABLE_WIDTH);
  mInvScaleTimeScaleFactor = 1.0f / mScaleTimeScaleFactor;

  timeIndex = 0;
  for (int i = 0; i < LOOKUP_TABLE_WIDTH; ++i) {
    glm::vec4 currentScale = glm::vec4(nodeAnim->mScalingKeys[timeIndex].mValue.x, nodeAnim->mScalingKeys[timeIndex].mValue.y, nodeAnim->mScalingKeys[timeIndex].mValue.z, 1.0f);
    glm::vec4 nextScale = glm::vec4(nodeAnim->mScalingKeys[timeIndex + 1].mValue.x, nodeAnim->mScalingKeys[timeIndex + 1].mValue.y, nodeAnim->mScalingKeys[timeIndex + 1].mValue.z, 1.0f);

    float currentKey = static_cast<float>(nodeAnim->mScalingKeys[timeIndex].mTime);
    float nextKey = static_cast<float>(nodeAnim->mScalingKeys[timeIndex + 1].mTime);
    float currentTime = i * mScaleTimeScaleFactor / scalingScaleFactor;

    mScalings.emplace_back(glm::mix(currentScale, nextScale, (currentTime - currentKey) / (nextKey - currentKey)));

    if (currentTime > nextKey) {
      if (timeIndex < mNumScalings - 1) {
        ++timeIndex;
      }
    }
  }

  mMinRotateTime = static_cast<float>(nodeAnim->mRotationKeys[0].mTime);
  mMaxRotateTime = static_cast<float>(nodeAnim->mRotationKeys[mNumRotations - 1].mTime);
  float rotateScaleFactor = maxClipDuration / mMaxRotateTime;
  mRotateTimeScaleFactor = maxClipDuration / static_cast<float>(LOOKUP_TABLE_WIDTH);
  mInvRotateTimeScaleFactor = 1.0f / mRotateTimeScaleFactor;

  timeIndex = 0;
  for (int i = 0; i < LOOKUP_TABLE_WIDTH; ++i) {
    glm::quat currentRotate = glm::quat(nodeAnim->mRotationKeys[timeIndex].mValue.w, nodeAnim->mRotationKeys[timeIndex].mValue.x,
                                        nodeAnim->mRotationKeys[timeIndex].mValue.y, nodeAnim->mRotationKeys[timeIndex].mValue.z);
    glm::quat nextRotate = glm::quat(nodeAnim->mRotationKeys[timeIndex + 1].mValue.w, nodeAnim->mRotationKeys[timeIndex + 1].mValue.x,
                                     nodeAnim->mRotationKeys[timeIndex + 1].mValue.y, nodeAnim->mRotationKeys[timeIndex + 1].mValue.z);

    float currentKey = static_cast<float>(nodeAnim->mRotationKeys[timeIndex].mTime);
    float nextKey = static_cast<float>(nodeAnim->mRotationKeys[timeIndex + 1].mTime);
    float currentTime = i * mRotateTimeScaleFactor / rotateScaleFactor;

    /* roiations are interpolated via SLERP */
    glm::quat rotation = glm::normalize(glm::slerp(currentRotate, nextRotate, (currentTime - currentKey) / (nextKey - currentKey)));

    /* we store the quaternion as glm::vec4 for the transport to the shader */
    mRotations.emplace_back(rotation.x, rotation.y, rotation.z, rotation.w);

    if (currentTime > nextKey) {
      if (timeIndex < mNumRotations - 1) {
        ++timeIndex;
      }
    }
  }
}

std::string AssimpAnimChannel::getTargetNodeName() {
  return mNodeName;
}

float AssimpAnimChannel::getMaxTime() {
  return std::max(std::max(mNumRotations, mNumTranslations), mNumScalings);
}

int AssimpAnimChannel::getBoneId() {
  return mBoneId;
}

void AssimpAnimChannel::setBoneId(unsigned int id) {
  mBoneId = id;
}

const std::vector<glm::vec4>& AssimpAnimChannel::getTranslationData() {
  return mTranslations;
}

const std::vector<glm::vec4>& AssimpAnimChannel::getScalingData() {
  return mScalings;
}

const std::vector<glm::vec4>& AssimpAnimChannel::getRotationData() {
  return mRotations;
}

float AssimpAnimChannel::getInvTranslationScaling() {
  return mInvTranslateTimeScaleFactor;
}

float AssimpAnimChannel::getInvRotationScaling() {
  return mInvRotateTimeScaleFactor;
}

float AssimpAnimChannel::getInvScaleScaling() {
  return mInvScaleTimeScaleFactor;
}
