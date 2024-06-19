#pragma once

#include <string>
#include <vector>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/anim.h>

class AssimpAnimChannel {
  public:
    void loadChannelData(aiNodeAnim* nodeAnim);
    std::string getTargetNodeName();
    float getMaxTime();

    glm::mat4 getTRSMatrix(float time);

    glm::vec3 getTranslation(float time);
    glm::vec3 getScaling(float time);
    glm::quat getRotation(float time);

  private:
    std::string mNodeName;

    /* use separate timinigs vectors, just in case not all keys have the same time */
    std::vector<float> mTranslationTiminngs{};
    std::vector<float> mInverseTranslationTimeDiffs{};
    std::vector<float> mRotationTiminigs{};
    std::vector<float> mInverseRotationTimeDiffs{};
    std::vector<float> mScaleTimings{};
    std::vector<float> mInverseScaleTimeDiffs{};

    /* every entry here has the same index as the timing for that key type */
    std::vector<glm::vec3> mTranslations{};
    std::vector<glm::vec3> mScalings{};
    std::vector<glm::quat> mRotations{};

    unsigned int mPreState = 0;
    unsigned int mPostState = 0;
};
