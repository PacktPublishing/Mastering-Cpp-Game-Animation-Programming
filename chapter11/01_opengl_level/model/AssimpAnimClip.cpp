#include "AssimpAnimClip.h"
#include "Logger.h"

void AssimpAnimClip::addChannels(aiAnimation* animation, float maxClipDuration, std::vector<std::shared_ptr<AssimpBone>> boneList) {
  mClipName = animation->mName.C_Str();
  mClipDuration = static_cast<float>(animation->mDuration);
  mClipTicksPerSecond = static_cast<float>(animation->mTicksPerSecond);

  Logger::log(1, "%s: - loading clip %s, duration %lf (%lf ticks per second)\n", __FUNCTION__, mClipName.c_str(), mClipDuration, mClipTicksPerSecond);

  for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
    std::shared_ptr<AssimpAnimChannel> channel = std::make_shared<AssimpAnimChannel>();

    channel->loadChannelData(animation->mChannels[i], maxClipDuration);

    std::string targetNodeName = channel->getTargetNodeName();
    const auto bonePos = std::find_if(boneList.begin(), boneList.end(),
      [targetNodeName](std::shared_ptr<AssimpBone> bone) { return bone->getBoneName() == targetNodeName; } );
    if (bonePos != boneList.end()) {
      channel->setBoneId((*bonePos)->getBoneId());
      Logger::log(1, "%s: -- loading channel %i for node '%s' in pos %i\n", __FUNCTION__, i, animation->mChannels[i]->mNodeName.C_Str(), channel->getBoneId());
    } else {
      Logger::log(1, "%s warning: skipping channel %i for node '%s'\n", __FUNCTION__, i, animation->mChannels[i]->mNodeName.C_Str());
    }

    mAnimChannels.emplace_back(channel);
  }
}

std::string AssimpAnimClip::getClipName() {
  return mClipName;
}

void AssimpAnimClip::setClipName(std::string name) {
  mClipName = name;
}

const std::vector<std::shared_ptr<AssimpAnimChannel>>& AssimpAnimClip::getChannels() {
  return mAnimChannels;
}

const unsigned int AssimpAnimClip::getNumChannels() {
  return mAnimChannels.size();
}

float AssimpAnimClip::getClipDuration() {
  return mClipDuration;
}

float AssimpAnimClip::getClipTicksPerSecond() {
  return mClipTicksPerSecond;
}
