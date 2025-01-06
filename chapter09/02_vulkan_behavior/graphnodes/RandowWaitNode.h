#pragma once

#include "GraphNodeBase.h"

class RandomWaitNode : public GraphNodeBase {
  public:
    RandomWaitNode(int nodeId, float minWaitTime = 3.0f, float maxWaitTime = 5.0f);
    explicit RandomWaitNode(const RandomWaitNode& orig);

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

    float getMaxWaitTime() { return mMaxWaitTime; };

    bool mActive = false;
    bool mFired = false;
    float mMinWaitTime = 0.0f;
    float mMaxWaitTime = 0.0f;
    float mCurrentTime = 0.0f;

    void calculateRandomWaitTime();
};
