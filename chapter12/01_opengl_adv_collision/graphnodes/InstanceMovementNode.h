#pragma once

#include <memory>

#include "GraphNodeBase.h"

#include "Enums.h"
#include "AssimpInstance.h"
#include "ModelInstanceCamData.h"

class InstanceMovementNode : public GraphNodeBase {
  public:
    InstanceMovementNode(int nodeId);

    virtual void draw(ModelInstanceCamData modInstCamData) override;
    virtual void update(float deltaTime) override {};
    virtual void activate() override;
    virtual void deactivate(bool informParentNodes) override {};
    virtual bool isActive() override { return false; };
    virtual std::shared_ptr<GraphNodeBase> clone() override;
    virtual std::optional<std::map<std::string, std::string>> exportData() override;
    virtual void importData(std::map<std::string, std::string> data) override;

  private:
    int mInId = 0;
    int mOutId = 0;
    int mStaticIdStart = 0;

    bool mSetState = false;
    bool mSetMoveDirection = false;
    bool mSetRotation = false;
    bool mSetSpeed = false;
    bool mSetPosition = false;

    moveDirection mMoveDir = moveDirection::none;
    moveState mMoveState = moveState::idle;

    float mRotation = 0.0f;
    float mMinRot = 0.0f;
    float mMaxRot = 0.0f;
    bool mRandomRot = false;
    bool mRandomRotChanged = !mRandomRot;
    bool mRelativeRot = true;

    float mSpeed = 1.0f;
    float mMinSpeed = 1.0f;
    float mMaxSpeed = 2.0f;
    bool mRandomSpeed = false;
    bool mRandomSpeedChanged = mRandomSpeed;;

    glm::vec3 mPosition = glm::vec3(0.0f);

    void calculateRotation();
    void calculateSpeed();
};
