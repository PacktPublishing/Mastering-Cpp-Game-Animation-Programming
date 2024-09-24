#pragma once

#include "GraphNodeBase.h"
#include "Enums.h"

class FaceAnimNode : public GraphNodeBase {
  public:
    FaceAnimNode(int nodeId);

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

    float mCurrentTime = 0.0f;
    float mCurrentBlendValue = 0.0f;

    faceAnimation mFaceAnim = faceAnimation::none;
    float mFaceAnimStartWeight = 0.0f;
    float mFaceAnimEndWeight = 1.0f;
    float mFaceAnimBlendTime = 1.0f;

    void resetTimes();
};
