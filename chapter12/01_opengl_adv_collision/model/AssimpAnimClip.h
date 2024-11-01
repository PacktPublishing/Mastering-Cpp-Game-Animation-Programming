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
    const std::shared_ptr<AssimpAnimChannel> getChannel(unsigned int index);

    const std::string& getClipName();
    const float& getClipDuration();
    const float& getClipTicksPerSecond();

    const unsigned int getNumChannels();

    void setClipName(std::string name);

  private:
    std::string mClipName;
    float mClipDuration = 0.0f;
    float mClipTicksPerSecond = 0.0f;

    std::vector<std::shared_ptr<AssimpAnimChannel>> mAnimChannels{};
};
