#pragma once

#include <vector>

#include "GraphNodeBase.h"

class SelectorNode : public GraphNodeBase {
  public:
    SelectorNode(int nodeId, float waitTime = 3.0f, int numOut = 3);

    virtual void draw(ModelInstanceCamData modInstCamData) override;
    virtual void update(float deltaTime) override;
    virtual void activate() override;
    virtual void deactivate(bool informParnetNodes ) override;
    virtual bool isActive() override { return mActive; };
    virtual std::shared_ptr<GraphNodeBase> clone() override;
    virtual std::optional<std::map<std::string, std::string>> exportData() override;
    virtual void importData(std::map<std::string, std::string> data) override;

    virtual void addOutputPin() override;
    virtual int delOutputPin() override;
    virtual int getNumOutputPins() override;

  private:
    int mInId = 0;
    int mStaticIdStart = 0;
    int mOutIdStart = 0;

    bool mActive = false;
    float mWaitTime = 0.0f;
    float mCurrentTime = 0.0f;

    std::vector<int> mOutIds{};
    int mActiveOut = -1;
};
