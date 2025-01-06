#include "AssimpAnimClip.h"
#include "Logger.h"

void AssimpAnimClip::addChannels(aiAnimation* animation, std::vector<std::shared_ptr<AssimpBone>> boneList) {
  mClipName = animation->mName.C_Str();
  mClipDuration = static_cast<float>(animation->mDuration);
  mClipTicksPerSecond = static_cast<float>(animation->mTicksPerSecond);

  Logger::log(1, "%s: - loading clip %s, duration %lf (%lf ticks per second)\n", __FUNCTION__, mClipName.c_str(), mClipDuration, mClipTicksPerSecond);

  for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
    std::shared_ptr<AssimpAnimChannel> channel = std::make_shared<AssimpAnimChannel>();

    Logger::log(1, "%s: -- loading channel %i for node '%s'\n", __FUNCTION__, i, animation->mChannels[i]->mNodeName.C_Str());
    channel->loadChannelData(animation->mChannels[i]);

    std::string targetNodeName = channel->getTargetNodeName();
    const auto bonePos = std::find_if(boneList.begin(), boneList.end(),
      [targetNodeName](std::shared_ptr<AssimpBone> bone) { return bone->getBoneName() == targetNodeName; } );
    if (bonePos != boneList.end()) {
      channel->setBoneId((*bonePos)->getBoneId());
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

float AssimpAnimClip::getClipDuration() {
  return mClipDuration;
}

float AssimpAnimClip::getClipTicksPerSecond() {
  return mClipTicksPerSecond;
}
