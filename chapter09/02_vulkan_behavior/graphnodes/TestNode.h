#pragma once

#include "GraphNodeBase.h"

class TestNode : public GraphNodeBase {
  public:
    TestNode(int nodeId);

    virtual void draw(ModelInstanceCamData modInstCamData) override;
    virtual void update(float deltaTime) override {};
    virtual void activate() override {};
    virtual void deactivate(bool informParentNodes) override {};
    virtual bool isActive() override { return false; };
    virtual std::shared_ptr<GraphNodeBase> clone() override;
    virtual std::optional<std::map<std::string, std::string>> exportData() override { return {}; };
    virtual void importData(std::map<std::string, std::string> data) override {};

  private:
    int mOutId = 0;
    int mStaticIdStart = 0;

    void fireOutput();
};
