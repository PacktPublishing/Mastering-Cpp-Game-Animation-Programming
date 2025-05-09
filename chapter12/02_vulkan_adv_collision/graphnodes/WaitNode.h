#pragma once

#include "GraphNodeBase.h"

class WaitNode : public GraphNodeBase {
  public:
    WaitNode(int nodeId, float waitTime = 3.0f);

    virtual void draw(ModelInstanceCamData modInstCamData) override;
    virtual void update(float deltaTime) override;
    virtual void activate() override;
    virtual void deactivate(bool informParentNodes = true) override;
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
    float mWaitTime = 0.0f;
    float mCurrentTime = 0.0f;
};
