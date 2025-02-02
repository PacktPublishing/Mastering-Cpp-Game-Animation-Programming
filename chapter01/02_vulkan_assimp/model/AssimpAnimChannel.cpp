#include "AssimpAnimChannel.h"

#include "Logger.h"

void AssimpAnimChannel::loadChannelData(aiNodeAnim* nodeAnim) {
  mNodeName = nodeAnim->mNodeName.C_Str();
  unsigned int numTranslations = nodeAnim->mNumPositionKeys;
  unsigned int numRotations = nodeAnim->mNumRotationKeys;
  unsigned int numScalings = nodeAnim->mNumScalingKeys;
  unsigned int preState = nodeAnim->mPreState;
  unsigned int postState = nodeAnim->mPostState;

  Logger::log(1, "%s: - loading animation channel for node '%s', with %i translation keys, %i rotation keys, %i scaling keys (preState %i, postState %i)\n",
              __FUNCTION__, mNodeName.c_str(), numTranslations, numRotations, numScalings, preState, postState);

  for (unsigned int i = 0; i < numTranslations; ++i) {
    mTranslationTiminngs.emplace_back(static_cast<float>(nodeAnim->mPositionKeys[i].mTime));
    mTranslations.emplace_back(glm::vec3(nodeAnim->mPositionKeys[i].mValue.x, nodeAnim->mPositionKeys[i].mValue.y, nodeAnim->mPositionKeys[i].mValue.z));
  }

  for (unsigned int i = 0; i < numRotations; ++i) {
    mRotationTiminigs.emplace_back(static_cast<float>(nodeAnim->mRotationKeys[i].mTime));
    mRotations.emplace_back(glm::quat(nodeAnim->mRotationKeys[i].mValue.w, nodeAnim->mRotationKeys[i].mValue.x, nodeAnim->mRotationKeys[i].mValue.y, nodeAnim->mRotationKeys[i].mValue.z));
  }

  for (unsigned int i = 0; i < numScalings; ++i) {
    mScaleTimings.emplace_back(static_cast<float>(nodeAnim->mScalingKeys[i].mTime));
    mScalings.emplace_back(glm::vec3(nodeAnim->mScalingKeys[i].mValue.x, nodeAnim->mScalingKeys[i].mValue.y, nodeAnim->mScalingKeys[i].mValue.z));
  }

  /* precalcuate the inverse offset to avoid divisions when scaling the section */
  for (unsigned int i = 0; i < mTranslationTiminngs.size() - 1; ++i) {
    mInverseTranslationTimeDiffs.emplace_back(1.0f / (mTranslationTiminngs.at(i + 1) - mTranslationTiminngs.at(i)));
  }
  for (unsigned int i = 0; i < mRotationTiminigs.size() - 1; ++i) {
    mInverseRotationTimeDiffs.emplace_back(1.0f / (mRotationTiminigs.at(i + 1) - mRotationTiminigs.at(i)));
  }
  for (unsigned int i = 0; i < mScaleTimings.size() - 1; ++i) {
    mInverseScaleTimeDiffs.emplace_back(1.0f / (mScaleTimings.at(i + 1) - mScaleTimings.at(i)));
  }

  mPreState = preState;
  mPostState = postState;
}

std::string AssimpAnimChannel::getTargetNodeName() {
  return mNodeName;
}

float AssimpAnimChannel::getMaxTime() {
  float maxTranslationTime = mTranslationTiminngs.at(mTranslationTiminngs.size() - 1);
  float maxRotationTime = mRotationTiminigs.at(mRotationTiminigs.size() - 1);
  float maxScaleTime = mScaleTimings.at(mScaleTimings.size() - 1);

  return std::max(std::max(maxRotationTime, maxTranslationTime), maxScaleTime);
}

/* precalculate TRS matrix */
glm::mat4 AssimpAnimChannel::getTRSMatrix(float time) {
  return glm::translate(glm::mat4_cast(getRotation(time)) * glm::scale(glm::mat4(1.0f), getScaling(time)), getTranslation(time));
}

glm::vec3 AssimpAnimChannel::getTranslation(float time) {
  if (mTranslations.empty()) {
    return glm::vec3(0.0f);
  }

  /* handle time before and after */
  switch (mPreState) {
    case 0:
      /* do not change vertex position-> aiAnimBehaviour_DEFAULT */
      if (time < mTranslationTiminngs.at(0)) {
        return glm::vec3(0.0f);
      }
      break;
    case 1:
      /* use value at zero time "aiAnimBehaviour_CONSTANT" */
      if (time < mTranslationTiminngs.at(0)) {
        return mTranslations.at(0);
      }
      break;
    default:
      Logger::log(1, "%s error: preState %i not implmented\n", __FUNCTION__, mPreState);
      break;
  }

  switch(mPostState) {
    case 0:
      if (time > mTranslationTiminngs.at(mTranslationTiminngs.size() - 1)) {
        return glm::vec3(0.0f);
      }
      break;
    case 1:
      if (time >= mTranslationTiminngs.at(mTranslationTiminngs.size() - 1)) {
        return mTranslations.at(mTranslations.size() - 1);
      }
      break;
    default:
      Logger::log(1, "%s error: postState %i not implmented\n", __FUNCTION__, mPostState);
      break;
  }

  auto timeIndexPos = std::lower_bound(mTranslationTiminngs.begin(), mTranslationTiminngs.end(), time);
  /* catch rare cases where time is exaclty zero */
  int timeIndex = std::max(static_cast<int>(std::distance(mTranslationTiminngs.begin(), timeIndexPos)) - 1, 0);

  float interpolatedTime = (time - mTranslationTiminngs.at(timeIndex)) * mInverseTranslationTimeDiffs.at(timeIndex);

  return glm::mix(mTranslations.at(timeIndex), mTranslations.at(timeIndex + 1), interpolatedTime);
}

glm::vec3 AssimpAnimChannel::getScaling(float time) {
  if (mScalings.empty()) {
    return glm::vec3(1.0f);
  }

  /* handle time before and after */
  switch (mPreState) {
    case 0:
      /* do not change vertex position-> aiAnimBehaviour_DEFAULT */
      if (time < mScaleTimings.at(0)) {
        return glm::vec3(0.0f);
      }
      break;
    case 1:
      /* use value at zero time "aiAnimBehaviour_CONSTANT" */
      if (time < mScaleTimings.at(0)) {
        return mScalings.at(0);
      }
      break;
    default:
      Logger::log(1, "%s error: preState %i not implmented\n", __FUNCTION__, mPreState);
      break;
  }

  switch(mPostState) {
    case 0:
      if (time > mScaleTimings.at(mScaleTimings.size() - 1)) {
        return glm::vec3(0.0f);
      }
      break;
    case 1:
      if (time >= mScaleTimings.at(mScaleTimings.size() - 1)) {
        return mScalings.at(mScalings.size() - 1);
      }
      break;
    default:
      Logger::log(1, "%s error: postState %i not implmented\n", __FUNCTION__, mPostState);
      break;
  }

  auto timeIndexPos = std::lower_bound(mScaleTimings.begin(), mScaleTimings.end(), time);
  int timeIndex = std::max(static_cast<int>(std::distance(mScaleTimings.begin(), timeIndexPos)) - 1, 0);

  float interpolatedTime = (time - mScaleTimings.at(timeIndex)) * mInverseScaleTimeDiffs.at(timeIndex);

  return glm::mix(mScalings.at(timeIndex), mScalings.at(timeIndex + 1), interpolatedTime);
}

glm::quat AssimpAnimChannel::getRotation(float time) {
  if (mRotations.empty()) {
    return glm::identity<glm::quat>();
  }

  /* handle time before and after */
  switch (mPreState) {
    case 0:
      /* do not change vertex position-> aiAnimBehaviour_DEFAULT */
      if (time < mRotationTiminigs.at(0)) {
        return glm::identity<glm::quat>();
      }
      break;
    case 1:
      /* use value at zero time "aiAnimBehaviour_CONSTANT" */
      if (time < mRotationTiminigs.at(0)) {
        return mRotations.at(0);
      }
      break;
    default:
      Logger::log(1, "%s error: preState %i not implmented\n", __FUNCTION__, mPreState);
      break;
  }

  switch(mPostState) {
    case 0:
      if (time > mRotationTiminigs.at(mRotationTiminigs.size() - 1)) {
        return glm::identity<glm::quat>();
      }
      break;
    case 1:
      if (time >= mRotationTiminigs.at(mRotationTiminigs.size() - 1)) {
        return mRotations.at(mRotations.size() - 1);
      }
      break;
    default:
      Logger::log(1, "%s error: postState %i not implmented\n", __FUNCTION__, mPostState);
      break;
  }

  auto timeIndexPos = std::lower_bound(mRotationTiminigs.begin(), mRotationTiminigs.end(), time);
  int timeIndex = std::max(static_cast<int>(std::distance(mRotationTiminigs.begin(), timeIndexPos)) - 1, 0);

  float interpolatedTime = (time - mRotationTiminigs.at(timeIndex)) * mInverseRotationTimeDiffs.at(timeIndex);

  /* roiations are interpolated via SLERP */
  return glm::normalize(glm::slerp(mRotations.at(timeIndex), mRotations.at(timeIndex + 1), interpolatedTime));
}
