#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class AssimpNode : public std::enable_shared_from_this<AssimpNode>  {
  public:
    static std::shared_ptr<AssimpNode> createNode(std::string nodeName);

    AssimpNode(std::string nodeName);

    std::shared_ptr<AssimpNode> addChild(std::string childNode);
    void addChilds(std::vector<std::string> childNodes);

    void setTranslation(glm::vec3 translation);
    void setRotation(glm::quat rotation);
    void setScaling(glm::vec3 scaling);

    void setRootTransformMatrix(glm::mat4 matrix);

    void updateTRSMatrix();
    glm::mat4 getTRSMatrix();

    std::string getNodeName();
    std::shared_ptr<AssimpNode> getParentNode();
    std::string getParentNodeName();

    std::vector<std::shared_ptr<AssimpNode>> getChilds();
    std::vector<std::string> getChildNames();

  private:
    std::string mNodeName = "(invalid)";
    std::weak_ptr<AssimpNode> mParentNode{};
    std::vector<std::shared_ptr<AssimpNode>> mChildNodes{};

    glm::vec3 mTranslation = glm::vec3(0.0f);
    glm::quat mRotation = glm::identity<glm::quat>();
    glm::vec3 mScaling = glm::vec3(1.0f);

    glm::mat4 mTranslationMatrix = glm::mat4(1.0f);
    glm::mat4 mRotationMatrix = glm::mat4(1.0f);
    glm::mat4 mScalingMatrix = glm::mat4(1.0f);

    glm::mat4 mParentNodeMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);

    /* extra matrix to move model instances  around */
    glm::mat4 mRootTransformMatrix = glm::mat4(1.0f);
};
