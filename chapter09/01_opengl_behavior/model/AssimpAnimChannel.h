#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/anim.h>

class AssimpAnimChannel {
  public:
    void loadChannelData(aiNodeAnim* nodeAnim, float maxClipDuration);
    std::string getTargetNodeName();
    float getMaxTime();

    const std::vector<glm::vec4>& getTranslationData();
    const std::vector<glm::vec4>& getRotationData();
    const std::vector<glm::vec4>& getScalingData();

    float getInvTranslationScaling();
    float getInvRotationScaling();
    float getInvScaleScaling();

    int getBoneId();
    void setBoneId(unsigned int id);

  private:
    std::string mNodeName;

    /* every entry here has the same index as the timing for that key type */
    std::vector<glm::vec4> mTranslations{};
    std::vector<glm::vec4> mScalings{};
    std::vector<glm::vec4> mRotations{};

    unsigned int mPreState = 0;
    unsigned int mPostState = 0;

    const int LOOKUP_TABLE_WIDTH = 1023;

    int mNumTranslations = 0;
    int mNumScalings = 0;
    int mNumRotations = 0;

    float mTranslateTimeScaleFactor = 0.0f;
    float mScaleTimeScaleFactor = 0.0f;
    float mRotateTimeScaleFactor = 0.0f;
    float mInvTranslateTimeScaleFactor = 0.0f;
    float mInvScaleTimeScaleFactor = 0.0f;
    float mInvRotateTimeScaleFactor = 0.0f;

    float mMinTranslateTime = 0.0f;
    float mMaxTranslateTime = 0.0f;
    float mMinScaleTime = 0.0f;
    float mMaxScaleTime = 0.0f;
    float mMinRotateTime = 0.0f;
    float mMaxRotateTime = 0.0f;

    int mBoneId = -1;
};
