#include "AssimpAnimClip.h"
#include "Logger.h"

void AssimpAnimClip::addChannels(aiAnimation* animation) {
  mClipName = animation->mName.C_Str();
  mClipDuration = animation->mDuration;
  mClipTicksPerSecond = animation->mTicksPerSecond;

  Logger::log(1, "%s: - loading clip %s, duration %lf (%lf ticks per second)\n", __FUNCTION__, mClipName.c_str(), mClipDuration, mClipTicksPerSecond);

  for (unsigned int i = 0; i < animation->mNumChannels; ++i) {
    std::shared_ptr<AssimpAnimChannel> channel = std::make_shared<AssimpAnimChannel>();

    Logger::log(1, "%s: -- loading channel %i for node '%s'\n", __FUNCTION__, i, animation->mChannels[i]->mNodeName.C_Str());
    channel->loadChannelData(animation->mChannels[i]);
    mAnimChannels.emplace_back(channel);
  }
}

std::string AssimpAnimClip::getClipName() {
  return mClipName;
}

void AssimpAnimClip::setClipName(std::string name) {
  mClipName = name;
}

std::vector<std::shared_ptr<AssimpAnimChannel>> AssimpAnimClip::getChannels() {
  return mAnimChannels;
}

float AssimpAnimClip::getClipDuration() {
  return mClipDuration;
}

float AssimpAnimClip::getClipTicksPerSecond() {
  return mClipTicksPerSecond;
}
