#include "AssimpBone.h"

#include "Logger.h"

AssimpBone::AssimpBone(unsigned int id, std::string name, glm::mat4 matrix) : mBoneId(id),  mNodeName(name), mOffsetMatrix(matrix) {
      Logger::log(1, "%s: --- added bone %i for node name '%s'\n", __FUNCTION__, mBoneId, mNodeName.c_str());
}

std::string AssimpBone::getBoneName() {
  return mNodeName;
}

unsigned int AssimpBone::getBoneId() {
  return mBoneId;
}

glm::mat4 AssimpBone::getOffsetMatrix() {
  return mOffsetMatrix;
}

