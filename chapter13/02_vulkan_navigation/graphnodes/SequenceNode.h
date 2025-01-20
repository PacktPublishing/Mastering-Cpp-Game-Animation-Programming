#pragma once

#include <vector>

#include "GraphNodeBase.h"

class SequenceNode : public GraphNodeBase {
  public:
    SequenceNode(int nodeId, int numOut = 3);

    virtual void draw(ModelInstanceCamData modInstCamData) override;
    virtual void update(float deltaTime) override {};
    virtual void activate() override;
    virtual void deactivate(bool informParentNodes) override;
    virtual bool isActive() override { return mActive; };
    virtual std::shared_ptr<GraphNodeBase> clone() override;
    virtual std::optional<std::map<std::string, std::string>> exportData() override;
    virtual void importData(std::map<std::string, std::string> data) override;

    virtual void addOutputPin() override;
    virtual int delOutputPin() override;
    virtual int getNumOutputPins() override;
    virtual void childFinishedExecution() override;

  private:
    int mInId = 0;
    int mOutIdStart = 0;

    bool mActive = false;
    std::vector<int> mOutIds{};
    int mActiveOut = -1;
};
