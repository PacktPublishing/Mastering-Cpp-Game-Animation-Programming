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
#include "BoundingBox2D.h"

class AssimpInstance {
  public:
    AssimpInstance(std::shared_ptr<AssimpModel> model, glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f), float modelScale = 1.0f);

    std::shared_ptr<AssimpModel> getModel();
    glm::vec3 getWorldPosition();
    glm::mat4 getWorldTransformMatrix();
    void updateModelRootMatrix();

    void setWorldPosition(glm::vec3 position);
    void setRotation(glm::vec3 rotation);
    void setScale(float scale);
    void setSwapYZAxis(bool value);

    glm::vec3 getRotation();
    float getScale();
    bool getSwapYZAxis();

    glm::vec3 get2DRotationVector();

    void setInstanceSettings(InstanceSettings settings);
    InstanceSettings getInstanceSettings();

    int getInstanceIndexPosition();
    int getInstancePerModelIndexPosition();

    void blendIdleWalkRunAnimation(float deltaTime);
    void playIdleWalkRunAnimation();

    void blendActionAnimation(float deltaTime, bool backwards = false);
    void playActionAnimation();

    void updateAnimation(float deltaTime);
    void updateInstanceState(moveState state, moveDirection dir);
    void updateInstanceSpeed(float deltaTime);
    void updateInstancePosition(float deltaTime);
    void setNextInstanceState(moveState state);

    void rotateInstance(float angle);
    void rotateInstance(glm::vec3 angles);

    void setForwardSpeed(float speed);
    void stopInstance();

    BoundingBox2D getBoundingBox();
    void setBoundingBox(BoundingBox2D box);

    void setFaceAnim(faceAnimation faceAnim);
    void setFaceAnimWeight(float weight);

    void setHeadAnim(glm::vec2 leftRightUpDownValues);

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

    /* calculated via glm::length */
    const float MAX_ACCEL = 4.0f;
    const float MAX_ABS_SPEED = 1.0f;
    const float MIN_STOP_SPEED = 0.01f;

    float mMaxInstanceSpeed = MAX_ABS_SPEED;

    moveDirection mPrevMoveDirection = moveDirection::none;

    moveState mNextMoveState = moveState::idle;
    moveState mActionMoveState = moveState::idle;

    bool mAnimRestarted = false;

    animationState mAnimState = animationState::playIdleWalkRun;
    void updateAnimStateMachine(float deltaTime);

    BoundingBox2D mBoundingBox{};
};
