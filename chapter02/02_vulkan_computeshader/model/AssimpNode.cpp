#include "AssimpNode.h"
#include "Logger.h"

AssimpNode::AssimpNode(std::string nodeName) : mNodeName(nodeName) {}

std::shared_ptr<AssimpNode> AssimpNode::createNode(std::string nodeName) {
  std::shared_ptr<AssimpNode> node = std::make_shared<AssimpNode>(nodeName);
  return node;
}

std::shared_ptr<AssimpNode> AssimpNode::addChild(std::string childName) {
  std::shared_ptr<AssimpNode> child = std::make_shared<AssimpNode>(childName);
  child->mParentNode = shared_from_this();

  Logger::log(1, "%s: -- adding child %s to parent %s\n", __FUNCTION__, childName.c_str(), child->getParentNodeName().c_str());
  mChildNodes.push_back(child);
  return child;
}

void AssimpNode::addChilds(std::vector<std::string> childNodes) {
  for (const auto& childName : childNodes) {
    std::shared_ptr<AssimpNode> child = std::make_shared<AssimpNode>(childName);
    child->mParentNode = shared_from_this();

    Logger::log(1, "%s: -- adding child %s to parent %s\n", __FUNCTION__, childName.c_str(), child->getParentNodeName().c_str());
    mChildNodes.push_back(child);
  }
}

void AssimpNode::setTranslation(glm::vec3 translation) {
  mTranslation = translation;
  mTranslationMatrix = glm::translate(glm::mat4(1.0f), mTranslation);
}

void AssimpNode::setRotation(glm::quat rotation) {
  mRotation = rotation;
  mRotationMatrix = glm::mat4_cast(mRotation);
}

void AssimpNode::setScaling(glm::vec3 scaling) {
  mScaling = scaling;
  mScalingMatrix = glm::scale(glm::mat4(1.0f), mScaling);
}

void AssimpNode::updateTRSMatrix() {
  if (std::shared_ptr<AssimpNode> parentNode = mParentNode.lock()) {
    mParentNodeMatrix = parentNode->getTRSMatrix();
  }

  mLocalTRSMatrix = mRootTransformMatrix * mParentNodeMatrix * mTranslationMatrix * mRotationMatrix * mScalingMatrix;
}

glm::mat4 AssimpNode::getTRSMatrix() {
  return mLocalTRSMatrix;
}

void AssimpNode::setRootTransformMatrix(glm::mat4 matrix) {
  mRootTransformMatrix = matrix;
}

std::string AssimpNode::getNodeName() {
  return mNodeName;
}

std::shared_ptr<AssimpNode> AssimpNode::getParentNode() {
  if (std::shared_ptr<AssimpNode> pNode = mParentNode.lock()) {
    return pNode;
  }
  return nullptr;
}

std::string AssimpNode::getParentNodeName() {
  if (std::shared_ptr<AssimpNode> pNode = mParentNode.lock()) {
    return pNode->getNodeName();
  }
  return std::string("(invalid)");
}

std::vector<std::shared_ptr<AssimpNode>> AssimpNode::getChilds() {
  return mChildNodes;
}

std::vector<std::string> AssimpNode::getChildNames() {
  std::vector<std::string> childNames{};
  for (const auto& childNode : mChildNodes) {
    childNames.push_back(childNode->getNodeName());
  }
  return childNames;
}
