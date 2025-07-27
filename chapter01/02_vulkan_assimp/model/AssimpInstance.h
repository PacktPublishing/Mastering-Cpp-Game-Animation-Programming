#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "AssimpModel.h"
#include "AssimpNode.h"
#include "AssimpBone.h"
#include "InstanceSettings.h"

class AssimpInstance {
  public:
    AssimpInstance(std::shared_ptr<AssimpModel> model, glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f), float modelScale = 1.0f);

    std::shared_ptr<AssimpModel> getModel();
    glm::vec3 getWorldPosition();
    glm::mat4 getWorldTransformMatrix();

    void setTranslation(glm::vec3 position);
    void setRotation(glm::vec3 rotation);
    void setScale(float scale);
    void setSwapYZAxis(bool value);

    glm::vec3 getTranslation();
    glm::vec3 getRotation();
    float getScale();
    bool getSwapYZAxis();

    std::vector<glm::mat4> getBoneMatrices();

    void setInstanceSettings(InstanceSettings settings);
    InstanceSettings getInstanceSettings();

    void updateModelRootMatrix();
    void updateAnimation(float deltaTime);

  private:
    std::shared_ptr<AssimpModel> mAssimpModel = nullptr;

    InstanceSettings mInstanceSettings{};

    glm::mat4 mLocalTranslationMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalRotationMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalScaleMatrix = glm::mat4(1.0f);
    glm::mat4 mLocalSwapAxisMatrix = glm::mat4(1.0f);

    glm::mat4 mLocalTransformMatrix = glm::mat4(1.0f);

    glm::mat4 mInstanceRootMatrix = glm::mat4(1.0f);
    glm::mat4 mModelRootMatrix = glm::mat4(1.0f);

    std::vector<glm::mat4> mBoneMatrices{};
};
