#pragma once

#include <string>

#include <imgui.h>
#include <imnodes.h>
#include <variant>
#include <memory>
#include <map>
#include <string>
#include <optional>

#include "Enums.h"
#include "Callbacks.h"
#include "ModelInstanceCamData.h"

class GraphNodeBase {
  public:
    GraphNodeBase(int nodeId);
    virtual ~GraphNodeBase() {};

    /* MUST be overridden */
    virtual void update(float deltaTime) = 0;
    virtual void draw(ModelInstanceCamData modInstCamData) = 0;
    virtual void activate() = 0;
    virtual void deactivate(bool informParentNodes = true) = 0;
    virtual bool isActive() = 0;
    virtual std::shared_ptr<GraphNodeBase> clone() = 0;
    virtual std::optional<std::map<std::string, std::string>> exportData() = 0;
    virtual void importData(std::map<std::string, std::string> data) = 0;

    /* CAN be overriden */
    virtual void addOutputPin() {};
    virtual int delOutputPin() { return 0; };
    virtual int getNumOutputPins() { return 0; };
    virtual void childFinishedExecution() {};
    virtual bool listensToEvent(nodeEvent event) { return false; };
    virtual void handleEvent() { };

    std::string getNodeName() { return mNodeName; }
    std::string getFormattedNodeName() { return mNodeName + " (" + std::to_string(mNodeId) + ")"; }
    int getNodeId() const { return mNodeId; }
    graphNodeType getNodeType() { return mNodeType; }

    void setNodeOutputTriggerCallback(fireNodeOutputCallback callbackFunction);
    void fireNodeOutputTriggerCallback(int outId);

    void setNodeActionCallback(nodeActionCallback callbackFunction);
    void fireNodeActionCallback(graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);

  private:
    int mNodeId = 0;
    std::string mNodeName;
    graphNodeType mNodeType = graphNodeType::none;

    fireNodeOutputCallback mNodeCallbackFunction;
    nodeActionCallback mNodeActionCallbackFunction;

  /* allow factory to set name directly  */
  friend class GraphNodeFactory;
};
