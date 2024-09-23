#pragma once

#include "GraphNodeBase.h"

class HeadAnimNode : public GraphNodeBase {
  public:
    HeadAnimNode(int nodeId);

    virtual void draw(ModelInstanceCamData modInstCamData) override;
    virtual void update(float deltaTime) override;
    virtual void activate() override;
    virtual void deactivate(bool informParentNodes) override;
    virtual bool isActive() override { return mActive; };
    virtual std::shared_ptr<GraphNodeBase> clone() override;
    virtual std::optional<std::map<std::string, std::string>> exportData() override;
    virtual void importData(std::map<std::string, std::string> data) override;

  private:
    int mInId = 0;
    int mOutId = 0;
    int mStaticIdStart = 0;

    bool mActive = false;
    bool mFired = false;

    bool mSetLeftRightHeadAnim = false;
    bool mSetUpDownHeadAnim = false;

    float mCurrentLeftRightBlendTime = 0.0f;
    float mCurrentLeftRightBlendValue = 0.0f;
    float mCurrentUpDownBlendTime = 0.0f;
    float mCurrentUpDownBlendValue = 0.0f;

    float mHeadMoveLeftRightStartWeight = 0.0f;
    float mHeadMoveLeftRightEndWeight = 0.0f;
    float mHeadMoveLeftRightBlendTime = 1.0f;

    float mHeadMoveUpDownStartWeight = 0.0f;
    float mHeadMoveUpDownEndWeight = 0.0f;
    float mHeadMoveUpDownBlendTime = 1.0f;

    void resetTimes();
};
