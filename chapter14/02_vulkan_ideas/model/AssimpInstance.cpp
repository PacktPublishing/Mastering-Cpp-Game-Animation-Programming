#include "AssimpInstance.h"

#include <algorithm>
#include <cmath> // atan2

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "ModelSettings.h"
#include "Logger.h"

AssimpInstance::AssimpInstance(std::shared_ptr<AssimpModel> model, glm::vec3 position, glm::vec3 rotation, float modelScale) : mAssimpModel(model) {
  if (!model) {
    Logger::log(1, "%s error: invalid model given\n", __FUNCTION__);
    return;
  }
  mInstanceSettings.isModelFile = model->getModelFileName();
  mInstanceSettings.isWorldPosition = position;
  mInstanceSettings.isWorldRotation = rotation;
  mInstanceSettings.isScale = modelScale;

  /* save model root matrix */
  mModelRootMatrix = mAssimpModel->getRootTranformationMatrix();

  updateModelRootMatrix();

  mBoundingBox = BoundingBox3D{
    glm::vec3(mInstanceSettings.isWorldPosition.x - 4.0f,
              mInstanceSettings.isWorldPosition.y - 4.0f,
              mInstanceSettings.isWorldPosition.z - 4.0f),
    { 8.0f, 8.0f, 8.0f }
  };
}

void AssimpInstance::updateModelRootMatrix() {
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
  mInstanceRootMatrix = mLocalTransformMatrix * mModelRootMatrix;
}

animationState AssimpInstance::getAnimState() {
  return mAnimState;
}

void AssimpInstance::updateAnimation(float deltaTime) {
  if (!mAssimpModel) {
    return;
  }

  mInstanceSettings.isFirstClipAnimPlayTimePos += deltaTime * mInstanceSettings.isAnimSpeedFactor * 1000.0f;

  /* check for a time rollover */
  mAnimRestarted = false;
  if (mInstanceSettings.isFirstClipAnimPlayTimePos >= mAssimpModel->getMaxClipDuration()) {
    mAnimRestarted = true;
  }

  mInstanceSettings.isFirstClipAnimPlayTimePos = std::fmod(mInstanceSettings.isFirstClipAnimPlayTimePos, mAssimpModel->getMaxClipDuration());
  updateAnimStateMachine(deltaTime);
}

void AssimpInstance::updateAnimStateMachine(float deltaTime) {
  ModelSettings modSettings = mAssimpModel->getModelSettings();

  moveState currentState = mInstanceSettings.isMoveState;
  moveState nextState = mNextMoveState;
  const auto stateChange = std::find_if(modSettings.msAllowedStateOrder.begin(), modSettings.msAllowedStateOrder.end(), [currentState, nextState](const auto& statePair) {
    return statePair.first == currentState && statePair.second == nextState;
  });

  switch (mAnimState) {
    case animationState::playIdleWalkRun:
      /* play idle/walk/run animation according to instance speed */
      /* move to next state if an action clip was requested */
      playIdleWalkRunAnimation();
      mInstanceSettings.isSecondClipAnimPlayTimePos = mInstanceSettings.isFirstClipAnimPlayTimePos;

      if (stateChange != modSettings.msAllowedStateOrder.end()) {
        /* save next state */
        mActionMoveState = mNextMoveState;
        Logger::log(2, "%s: going to state %i\n", __FUNCTION__, mActionMoveState);

        IdleWalkRunBlending blend;
        if (modSettings.msIWRBlendings.count(mInstanceSettings.isMoveDirection) > 0 ) {
          blend = modSettings.msIWRBlendings[mInstanceSettings.isMoveDirection];
        } else if (modSettings.msIWRBlendings.count(mPrevMoveDirection) > 0 ) {
          blend = modSettings.msIWRBlendings[mPrevMoveDirection];
        } else if (modSettings.msIWRBlendings.count(moveDirection::any) > 0) {
          blend = modSettings.msIWRBlendings[moveDirection::any];
        } else if (modSettings.msIWRBlendings.count(moveDirection::none) > 0) {
          blend = modSettings.msIWRBlendings[moveDirection::none];
        } else {
          /* no animation configured, jump to next state... */
          mAnimState = animationState::transitionFromIdleWalkRun;
          break;
        }

        float instanceSpeed = glm::length(mInstanceSettings.isSpeed);
        if (instanceSpeed <= MIN_STOP_SPEED) {
          mInstanceSettings.isFirstAnimClipNr = blend.iwrbIdleClipNr;
          mInstanceSettings.isSecondAnimClipNr = blend.iwrbIdleClipNr;
          mInstanceSettings.isAnimSpeedFactor = blend.iwrbIdleClipSpeed;
        } else if (instanceSpeed <= 1.0f) {
          mInstanceSettings.isFirstAnimClipNr = blend.iwrbWalkClipNr;
          mInstanceSettings.isSecondAnimClipNr = blend.iwrbWalkClipNr;
          mInstanceSettings.isAnimSpeedFactor = blend.iwrbWalkClipSpeed;
        } else {
          mInstanceSettings.isFirstAnimClipNr = blend.iwrbRunClipNr;
          mInstanceSettings.isSecondAnimClipNr = blend.iwrbRunClipNr;
          mInstanceSettings.isAnimSpeedFactor = blend.iwrbRunClipSpeed;
        }

        mInstanceSettings.isAnimBlendFactor = 0.0f;
        mInstanceSettings.isSecondClipAnimPlayTimePos = 0.0f;
        mAnimState = animationState::transitionFromIdleWalkRun;

        /* stop instance if the state is set to idle */
        if (stateChange->first == moveState::idle) {
          mInstanceSettings.isAccel = glm::vec3(0.0f);
          mInstanceSettings.isSpeed = glm::vec3(0.0f);
        }
      }
      break;
    case animationState::transitionFromIdleWalkRun:
      /* finish current idle/walk/run clip to be back in initial pose
       * this step was added to have a smooth transition
       * and not blend in the middle of an animation */
      /* skips at clip end to 'transitionToAction' */
      blendIdleWalkRunAnimation(deltaTime);
      break;
    case animationState::transitionToAction:
      /* blend between idle/walk/run and desired action */
      /* skips at clip end to 'playActionAnim' */
      blendActionAnimation(deltaTime);
      break;
    case animationState::playActionAnim:
      /* play and possibly repeat the desired action animation */
      playActionAnimation();
      /* skip only to next state when action animation clip was finished */
      if (mNextMoveState != mActionMoveState && mAnimRestarted) {
        mInstanceSettings.isAnimBlendFactor = 1.0f;
        mAnimState = animationState::transitionToIdleWalkRun;
      }
      break;
    case animationState::transitionToIdleWalkRun:
      /* blend between action and idle/walk/run by doing a backwards blend
       *from action to idle/walk/run clip */
      /* skips at clip end to 'playIdleWalkRun' */
      blendActionAnimation(deltaTime, true);
      break;
  }
}

void AssimpInstance::updateInstanceState(moveState state, moveDirection dir) {
  mInstanceSettings.isMoveKeyPressed = false;

  if (state == moveState::walk || state == moveState::run) {
    if ((dir & moveDirection::forward) == moveDirection::forward) {
      mInstanceSettings.isMoveKeyPressed = true;
      mInstanceSettings.isAccel.x = 5.0f;
    }
    if ((dir & moveDirection::back) == moveDirection::back) {
      mInstanceSettings.isMoveKeyPressed = true;
      mInstanceSettings.isAccel.x = -5.0f;
    }

    if ((dir & moveDirection::left) == moveDirection::left) {
      mInstanceSettings.isMoveKeyPressed = true;
      mInstanceSettings.isAccel.z = 5.0f;
    }
    if ((dir & moveDirection::right) == moveDirection::right) {
      mInstanceSettings.isMoveKeyPressed = true;
      mInstanceSettings.isAccel.z = -5.0f;
    }
  }

  if (mInstanceSettings.isMoveDirection != dir) {
    mPrevMoveDirection = mInstanceSettings.isMoveDirection;
    mInstanceSettings.isMoveDirection = dir;
  }

  if (mInstanceSettings.isMoveState != state) {
    mInstanceSettings.isMoveState = state;
  }
}

void AssimpInstance::setForwardSpeed(float speed) {
  mMaxInstanceSpeed = speed;
}

void AssimpInstance::stopInstance() {
  updateInstanceState(moveState::idle, moveDirection::none);
  setNextInstanceState(moveState::idle);
}

void AssimpInstance::updateInstanceSpeed(float deltaTime) {
  float currentSpeed = glm::length(mInstanceSettings.isSpeed);

  /* limit to max speed */
  float maxSpeed = mMaxInstanceSpeed;

  if (!mInstanceSettings.isMoveKeyPressed) {
    /* decelerate */
    if (currentSpeed > 0.0f) {
      if (mInstanceSettings.isSpeed.x > 0.0f) {
        mInstanceSettings.isAccel.x = -2.5f;
      }
      if (mInstanceSettings.isSpeed.x < 0.0f) {
        mInstanceSettings.isAccel.x = 2.5f;
      }
      if (mInstanceSettings.isSpeed.z > 0.0f) {
        mInstanceSettings.isAccel.z = -2.5f;
      }
      if (mInstanceSettings.isSpeed.z < 0.0f) {
        mInstanceSettings.isAccel.z = 2.5f;
      }
    }

    /* below minimal speed => halt */
    if (currentSpeed < MIN_STOP_SPEED) {
      currentSpeed = 0.0f;
      mInstanceSettings.isAccel = glm::vec3(0.0f);
      mInstanceSettings.isSpeed = glm::vec3(0.0f);
      /* do not force idle state in every update */
      // mInstanceSettings.isMoveState = moveState::idle;
      mInstanceSettings.isMoveDirection = moveDirection::none;
      mPrevMoveDirection = moveDirection::none;
    }
  }

  /* check for max accel */
  float currentAccel = glm::length(mInstanceSettings.isAccel);
  if (currentAccel > MAX_ACCEL) {
    mInstanceSettings.isAccel = glm::normalize(mInstanceSettings.isAccel);
    mInstanceSettings.isAccel *= MAX_ACCEL;
  }

  mInstanceSettings.isSpeed += mInstanceSettings.isAccel * deltaTime;

  /* recalulcate speed */
  currentSpeed = glm::length(mInstanceSettings.isSpeed);

  /* run -> double max speed  */
  if (mInstanceSettings.isMoveState == moveState::run) {
    maxSpeed = mMaxInstanceSpeed * 2.0f;
  }

  if (currentSpeed > maxSpeed) {
    if (mInstanceSettings.isMoveState != moveState::run) {
      /* we may come from run state, lower speed gradually */
      maxSpeed -= glm::length(mInstanceSettings.isAccel) * deltaTime;
      if (maxSpeed <= MAX_ABS_SPEED) {
        maxSpeed = MAX_ABS_SPEED;
      }
    }

    /* create unit vector */
    mInstanceSettings.isSpeed = glm::normalize(mInstanceSettings.isSpeed);
    /* and stretch again to max length */
    mInstanceSettings.isSpeed *= maxSpeed;
  }
}

void AssimpInstance::updateInstancePosition(float deltaTime) {
  if (!mInstanceSettings.isNoMovement) {
    ModelSettings modSettings = mAssimpModel->getModelSettings();
    /* rotate accel/speed according to instance azimuth -> WASD */
    float sinRot = std::sin(glm::radians(mInstanceSettings.isWorldRotation.y)) * modSettings.msForwardSpeedFactor;
    float cosRot = std::cos(glm::radians(mInstanceSettings.isWorldRotation.y)) * modSettings.msForwardSpeedFactor;
    float xSpeed = mInstanceSettings.isSpeed.x * sinRot + mInstanceSettings.isSpeed.z * cosRot;
    float zSpeed = mInstanceSettings.isSpeed.x * cosRot - mInstanceSettings.isSpeed.z * sinRot;

    /* scale speed by scaling factor of the instance */
    float speedFactor = mInstanceSettings.isScale;

    mInstanceSettings.isWorldPosition.z += zSpeed * speedFactor * deltaTime;
    mInstanceSettings.isWorldPosition.x += xSpeed * speedFactor * deltaTime;
  }

  /* set root node transform matrix, enabling instance movement */
  updateModelRootMatrix();
}

void AssimpInstance::applyGravity(float deltaTime) {
  /* apply gravity */
  glm::vec3 gravity = glm::vec3(0.0f, GRAVITY_CONSTANT * deltaTime, 0.0f);

  /* we are off ground on hop and jump, do not apply gravity */
  if (mInstanceSettings.isMoveState != moveState::hop && mInstanceSettings.isMoveState != moveState::jump) {
    if (!mInstanceSettings.isInstanceOnGround) {
      mInstanceSettings.isWorldPosition -= gravity;
    }
  }
}

void AssimpInstance::rotateInstance(float angle) {
  mInstanceSettings.isWorldRotation.y -= angle;
  if (mInstanceSettings.isWorldRotation.y < -180.0f) {
     mInstanceSettings.isWorldRotation.y += 360.0f;
  }
  if (mInstanceSettings.isWorldRotation.y >= 180.0f) {
    mInstanceSettings.isWorldRotation.y -= 360.0f;
  }
  updateModelRootMatrix();
}

void AssimpInstance::rotateInstance(glm::vec3 angles) {
  /* keep all angles between -180 and 180 degree */
  if (angles.x < -180.0f) {
   angles.x += 360.0f;
  }
  if (angles.x >= 180.0f) {
    angles.x -= 360.0f;
  }

  if (angles.y < -180.0f) {
   angles.y += 360.0f;
  }
  if (angles.y >= 180.0f) {
    angles.y -= 360.0f;
  }

  if (angles.z < -180.0f) {
   angles.z += 360.0f;
  }
  if (angles.z >= 180.0f) {
    angles.z -= 360.0f;
  }

  mInstanceSettings.isWorldRotation = angles;
  updateModelRootMatrix();
}

void AssimpInstance::rotateTo(glm::vec3 targetPos, float deltaTime) {
  /* only rotate when walk or run */
  if (mInstanceSettings.isMoveState != moveState::walk && mInstanceSettings.isMoveState != moveState::run) {
    return;
  }

  glm::vec3 myRotation = get2DRotationVector();

  glm::vec3 twoDimWorldPos = glm::vec3(mInstanceSettings.isWorldPosition.x, 0.0f, mInstanceSettings.isWorldPosition.z);
  glm::vec3 toTarget = glm::normalize(glm::vec3(targetPos.x, 0.0f, targetPos.z) - twoDimWorldPos);

  float angleDiff = glm::degrees(std::acos(glm::dot(myRotation, toTarget)));
  Logger::log(2, "%s: angle to target is %f degree\n", __FUNCTION__, angleDiff);

  /* do not rotate exactly to target, keep some deadzone */
  if (angleDiff > 6.0f) {
    glm::quat destRoation = glm::rotation(toTarget, myRotation);
    glm::vec3 angles = glm::eulerAngles(destRoation);

    rotateInstance(glm::degrees(angles.y) * deltaTime * 2.0f);
  }
}

void AssimpInstance::blendActionAnimation(float deltaTime, bool backwards) {
  ModelSettings modSettings = mAssimpModel->getModelSettings();
  IdleWalkRunBlending blend;
  if (modSettings.msIWRBlendings.count(mInstanceSettings.isMoveDirection) > 0 ) {
    blend = modSettings.msIWRBlendings[mInstanceSettings.isMoveDirection];
  } else if (modSettings.msIWRBlendings.count(mPrevMoveDirection) > 0 ) {
    blend = modSettings.msIWRBlendings[mPrevMoveDirection];
  } else if (modSettings.msIWRBlendings.count(moveDirection::any) > 0) {
    blend = modSettings.msIWRBlendings[moveDirection::any];
  } else if (modSettings.msIWRBlendings.count(moveDirection::none) > 0) {
    blend = modSettings.msIWRBlendings[moveDirection::none];
  } else {
    /* no animation configured, jump to next state... */
    if (backwards) {
      mAnimState = animationState::playIdleWalkRun;
    } else {
      mAnimState = animationState::playActionAnim;
    }
    return;
  }

  float blendSpeedFactor = deltaTime;
  float instanceSpeed = glm::length(mInstanceSettings.isSpeed);
  if (instanceSpeed <= MIN_STOP_SPEED) {
    mInstanceSettings.isFirstAnimClipNr = blend.iwrbIdleClipNr;
    blendSpeedFactor *= 15;
  } else if (instanceSpeed <= 1.0f) {
    mInstanceSettings.isFirstAnimClipNr = blend.iwrbWalkClipNr;
    blendSpeedFactor *= 20;
  } else {
    mInstanceSettings.isFirstAnimClipNr = blend.iwrbRunClipNr;
    blendSpeedFactor *= 25;
  }

  mInstanceSettings.isSecondAnimClipNr = modSettings.msActionClipMappings[mActionMoveState].aaClipNr;
  float animSpeed = modSettings.msActionClipMappings[mActionMoveState].aaClipSpeed;

  if (backwards) {
    mInstanceSettings.isAnimBlendFactor -= blendSpeedFactor;

    if (mInstanceSettings.isAnimBlendFactor <= 0.0f) {
      mAnimState = animationState::playIdleWalkRun;
      mNextMoveState = moveState::idle;
    }
  } else {
    mInstanceSettings.isAnimBlendFactor += blendSpeedFactor;

    if (mInstanceSettings.isAnimBlendFactor >= 1.0f) {
      mInstanceSettings.isFirstAnimClipNr = modSettings.msActionClipMappings[mActionMoveState].aaClipNr;
      mInstanceSettings.isAnimBlendFactor = 0.0f;
      mAnimState = animationState::playActionAnim;
    }
  }

  mInstanceSettings.isAnimSpeedFactor = glm::mix(blend.iwrbRunClipSpeed, animSpeed, mInstanceSettings.isAnimBlendFactor);
}

void AssimpInstance::playActionAnimation() {
  ModelSettings modSettings = mAssimpModel->getModelSettings();
  if (modSettings.msActionClipMappings.count(mActionMoveState) == 0) {
    return;
  }

  mInstanceSettings.isFirstAnimClipNr = modSettings.msActionClipMappings[mActionMoveState].aaClipNr;
  mInstanceSettings.isAnimSpeedFactor = modSettings.msActionClipMappings[mActionMoveState].aaClipSpeed;
  mInstanceSettings.isMoveState = mActionMoveState;
}

void AssimpInstance::blendIdleWalkRunAnimation(float deltaTime) {
  mInstanceSettings.isAnimBlendFactor += deltaTime * 5.0f;

  if (mInstanceSettings.isAnimBlendFactor >= 1.0f) {
    mInstanceSettings.isFirstClipAnimPlayTimePos = 0.0f;
    mInstanceSettings.isAnimBlendFactor = 0.0f;
    mAnimState = animationState::transitionToAction;
  }
}

void AssimpInstance::playIdleWalkRunAnimation() {
  ModelSettings modSettings = mAssimpModel->getModelSettings();
  IdleWalkRunBlending blend;

  /* do not play any animation in preview mode, use values from UI */
  if (modSettings.msPreviewMode) {
    return;
  }

  if (modSettings.msIWRBlendings.count(mInstanceSettings.isMoveDirection) > 0 ) {
    blend = modSettings.msIWRBlendings[mInstanceSettings.isMoveDirection];
  } else if (modSettings.msIWRBlendings.count(mPrevMoveDirection) > 0 ) {
    blend = modSettings.msIWRBlendings[mPrevMoveDirection];
  } else if (modSettings.msIWRBlendings.count(moveDirection::any) > 0) {
    blend = modSettings.msIWRBlendings[moveDirection::any];
  } else if (modSettings.msIWRBlendings.count(moveDirection::none) > 0) {
    blend = modSettings.msIWRBlendings[moveDirection::none];
  } else {
    /* no animation configured... */
    return;
  }

  float instanceSpeed = glm::length(mInstanceSettings.isSpeed);
  if (instanceSpeed <= 1.0f) {
    mInstanceSettings.isFirstAnimClipNr = blend.iwrbIdleClipNr;
    mInstanceSettings.isSecondAnimClipNr = blend.iwrbWalkClipNr;
    mInstanceSettings.isAnimSpeedFactor = glm::mix(blend.iwrbIdleClipSpeed, blend.iwrbWalkClipSpeed, instanceSpeed);
    mInstanceSettings.isAnimBlendFactor = instanceSpeed;
  } else {
    mInstanceSettings.isFirstAnimClipNr = blend.iwrbWalkClipNr;
    mInstanceSettings.isSecondAnimClipNr = blend.iwrbRunClipNr;
    mInstanceSettings.isAnimSpeedFactor = glm::mix(blend.iwrbWalkClipSpeed, blend.iwrbRunClipSpeed, instanceSpeed - 1.0f);
    mInstanceSettings.isAnimBlendFactor = 1.0f;
  }
}

void AssimpInstance::setNextInstanceState(moveState state) {
  mNextMoveState = state;
}

std::shared_ptr<AssimpModel> AssimpInstance::getModel() {
  return mAssimpModel;
}

glm::vec3 AssimpInstance::getWorldPosition() {
  return mInstanceSettings.isWorldPosition;
}

glm::mat4 AssimpInstance::getWorldTransformMatrix() {
  return mInstanceRootMatrix;
}

void AssimpInstance::setWorldPosition(glm::vec3 position) {
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

glm::vec3 AssimpInstance::get2DRotationVector() {
  /* get a vector in X-Z plane with the Y rotation */
  float sinRot = std::sin(glm::radians(mInstanceSettings.isWorldRotation.y));
  float cosRot = std::cos(glm::radians(mInstanceSettings.isWorldRotation.y));
  return glm::normalize(glm::vec3(sinRot, 0.0f, cosRot));
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

int AssimpInstance::getInstanceIndexPosition() {
  return mInstanceSettings.isInstanceIndexPosition;
}

int AssimpInstance::getInstancePerModelIndexPosition() {
  return mInstanceSettings.isInstancePerModelIndexPosition;
}

BoundingBox3D AssimpInstance::getBoundingBox() {
  return mBoundingBox;
}

void AssimpInstance::setBoundingBox(BoundingBox3D box) {
  mBoundingBox = box;
}

void AssimpInstance::setFaceAnim(faceAnimation faceAnim) {
  /* skip if model has no morph anims */
  if (!mAssimpModel->hasAnimMeshes()) {
    return;
  }

  if (faceAnim == faceAnimation::none) {
    /* reset weigth only when disabling animation */
    mInstanceSettings.isFaceAnimWeight = 0.0f;
  }
  mInstanceSettings.isFaceAnimType = faceAnim;
}

void AssimpInstance::setFaceAnimWeight(float weight) {
  if (mInstanceSettings.isFaceAnimType == faceAnimation::none) {
    return;
  }
  mInstanceSettings.isFaceAnimWeight = std::clamp(weight, 0.0f, 1.0f);
}

void AssimpInstance::setHeadAnim(glm::vec2 leftRightUpDownValues) {
  mInstanceSettings.isHeadLeftRightMove = leftRightUpDownValues.x;
  mInstanceSettings.isHeadUpDownMove = leftRightUpDownValues.y;
}

void AssimpInstance::setInstanceOnGround(bool value) {
  mInstanceSettings.isInstanceOnGround = value;
}

void AssimpInstance::setCollidingTriangles(std::vector<MeshTriangle>& collidingTriangles) {
  mInstanceSettings.isCollidingTriangles = collidingTriangles;
}

void AssimpInstance::setCurrentGroundTriangleIndex(int index) {
  mInstanceSettings.isCurrentGroundTriangleIndex = index;
}

void AssimpInstance::setNeighborGroundTriangleIndices(std::vector<int> indices) {
  mInstanceSettings.isNeighborGroundTriangles = indices;
}

int AssimpInstance::getCurrentGroundTriangleIndex() {
  return mInstanceSettings.isCurrentGroundTriangleIndex;
}

void AssimpInstance::setNavigationEnabled(bool value) {
  mInstanceSettings.isNavigationEnabled = value;
}

bool AssimpInstance::isNavigationEnabled() {
  return mInstanceSettings.isNavigationEnabled;
}

void AssimpInstance::setPathStartTriIndex(int index) {
  mInstanceSettings.isPathStartTriangleIndex = index;
}

void AssimpInstance::setPathTargetTriIndex(int index) {
  mInstanceSettings.isPathTargetTriangleIndex = index;
}

int AssimpInstance::getPathTargetTriIndex() {
  return mInstanceSettings.isPathTargetTriangleIndex;
}

void AssimpInstance::setPathTargetInstanceId(int index) {
  mInstanceSettings.isPathTargetInstance = index;
}

void AssimpInstance::setPathToTarget(std::vector<int> indices) {
  mInstanceSettings.isPathToTarget = indices;
}

std::vector<int> AssimpInstance::getPathToTarget() {
  return mInstanceSettings.isPathToTarget;
}
