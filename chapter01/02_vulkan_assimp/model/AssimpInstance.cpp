#include "AssimpInstance.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Logger.h"

AssimpInstance::AssimpInstance(std::shared_ptr<AssimpModel> model, glm::vec3 position, glm::vec3 rotation, float modelScale) : mAssimpModel(model) {
  if (!model) {
    Logger::log(1, "%s error: invalid model given\n", __FUNCTION__);
    return;
  }
  mInstanceSettings.isWorldPosition = position;
  mInstanceSettings.isWorldRotation = rotation;
  mInstanceSettings.isScale = modelScale;

  /* we need one 4x4 matrix for every bone */
  mBoneMatrices.resize(mAssimpModel->getBoneList().size());
  std::fill(mBoneMatrices.begin(), mBoneMatrices.end(), glm::mat4(1.0f));

  updateModelRootMatrix();
}

void AssimpInstance::updateModelRootMatrix() {
  if (!mAssimpModel) {
    Logger::log(1, "%s error: invalid model\n", __FUNCTION__);
    return;
  }

  mLocalScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(mInstanceSettings.isScale));

  if (mInstanceSettings.isSwapYZAxis) {
    glm::mat4 flipMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mLocalSwapAxisMatrix = glm::rotate(flipMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  } else {
    mLocalSwapAxisMatrix = glm::mat4(1.0f);
  }

  mLocalRotationMatrix = glm::mat4_cast(glm::quat(glm::radians(mInstanceSettings.isWorldRotation)));

  mLocalTranslationMatrix = glm::translate(glm::mat4(1.0f), mInstanceSettings.isWorldPosition);

  mLocalTransformMatrix = mLocalTranslationMatrix * mLocalRotationMatrix * mLocalSwapAxisMatrix * mLocalScaleMatrix;
}

void AssimpInstance::updateAnimation(float deltaTime) {
  mInstanceSettings.isAnimPlayTimePos += deltaTime * mAssimpModel->getAnimClips().at(mInstanceSettings.isAnimClipNr)->getClipTicksPerSecond() * mInstanceSettings.isAnimSpeedFactor;
  mInstanceSettings.isAnimPlayTimePos = std::fmod(mInstanceSettings.isAnimPlayTimePos, mAssimpModel->getAnimClips().at(mInstanceSettings.isAnimClipNr)->getClipDuration());

  std::vector<std::shared_ptr<AssimpAnimChannel>> animChannels = mAssimpModel->getAnimClips().at(mInstanceSettings.isAnimClipNr)->getChannels();

  /* animate clip via channels */
  for (const auto& channel : animChannels) {
    std::string nodeNameToAnimate = channel->getTargetNodeName();
    std::shared_ptr<AssimpNode> node = mAssimpModel->getNodeMap().at(nodeNameToAnimate);

    node->setRotation(channel->getRotation(mInstanceSettings.isAnimPlayTimePos));
    node->setScaling(channel->getScaling(mInstanceSettings.isAnimPlayTimePos));
    node->setTranslation(channel->getTranslation(mInstanceSettings.isAnimPlayTimePos));
  }

  /* set root node transform matrix, enabling instance movement */
  mAssimpModel->getRootNode()->setRootTransformMatrix(mLocalTransformMatrix * mAssimpModel->getRootTranformationMatrix());

  /* flat node map contains nodes in parent->child order, starting with root node, update matrices down the skeleton tree */
  mBoneMatrices.clear();
  for (auto& node : mAssimpModel->getNodeList()) {
    std::string nodeName = node->getNodeName();

    node->updateTRSMatrix();
    if (mAssimpModel->getBoneOffsetMatrices().count(nodeName) > 0) {
      mBoneMatrices.emplace_back(mAssimpModel->getNodeMap().at(nodeName)->getTRSMatrix() * mAssimpModel->getBoneOffsetMatrices().at(nodeName));
    }
  }
}

std::shared_ptr<AssimpModel> AssimpInstance::getModel() {
  return mAssimpModel;
}

glm::vec3 AssimpInstance::getWorldPosition() {
  return mInstanceSettings.isWorldPosition;
}

glm::mat4 AssimpInstance::getWorldTransformMatrix() {
  return mLocalTransformMatrix;
}

void AssimpInstance::setTranslation(glm::vec3 position) {
  mInstanceSettings.isWorldPosition = position;
  updateModelRootMatrix();
}

void AssimpInstance::setRotation(glm::vec3 rotation) {
  mInstanceSettings.isWorldRotation = rotation;
  updateModelRootMatrix();
}

void AssimpInstance::setScale(float scale) {
  mInstanceSettings.isScale = scale;
  updateModelRootMatrix();
}

void AssimpInstance::setSwapYZAxis(bool value) {
  mInstanceSettings.isSwapYZAxis = value;
  updateModelRootMatrix();
}

glm::vec3 AssimpInstance::getRotation() {
  return mInstanceSettings.isWorldRotation;
}

glm::vec3 AssimpInstance::getTranslation() {
  return mInstanceSettings.isWorldPosition;
}

float AssimpInstance::getScale() {
  return mInstanceSettings.isScale;
}

bool AssimpInstance::getSwapYZAxis() {
  return mInstanceSettings.isSwapYZAxis;
}

void AssimpInstance::setInstanceSettings(InstanceSettings settings) {
  mInstanceSettings = settings;
  updateModelRootMatrix();
}

InstanceSettings AssimpInstance::getInstanceSettings() {
  return mInstanceSettings;
}

std::vector<glm::mat4> AssimpInstance::getBoneMatrices() {
  return mBoneMatrices;
}
