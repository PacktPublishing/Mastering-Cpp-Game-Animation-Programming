#pragma once

#include <string>
#include <vector>
#include <memory>

#include <assimp/anim.h>

#include "AssimpAnimChannel.h"
#include "AssimpBone.h"

class AssimpAnimClip {
  public:
    void addChannels(aiAnimation* animation, float maxClipDuration, std::vector<std::shared_ptr<AssimpBone>> boneList);
    const std::vector<std::shared_ptr<AssimpAnimChannel>>& getChannels();

    std::string getClipName();
    float getClipDuration();
    float getClipTicksPerSecond();

    const unsigned int getNumChannels();

    void setClipName(std::string name);

  private:
    std::string mClipName;
    float mClipDuration = 0.0f;
    float mClipTicksPerSecond = 0.0f;

    std::vector<std::shared_ptr<AssimpAnimChannel>> mAnimChannels{};
};
