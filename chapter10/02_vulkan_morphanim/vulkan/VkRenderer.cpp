#include <imgui_impl_glfw.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <filesystem>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VkRenderer.h"

#include "Framebuffer.h"
#include "SelectionFramebuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "SyncObjects.h"
#include "Renderpass.h"
#include "SecondaryRenderpass.h"
#include "SelectionRenderpass.h"

#include "PipelineLayout.h"
#include "SkinningPipeline.h"
#include "ComputePipeline.h"
#include "LinePipeline.h"

#include "InstanceSettings.h"
#include "AssimpSettingsContainer.h"
#include "YamlParser.h"

#include "Logger.h"

VkRenderer::VkRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;
}

bool VkRenderer::init(unsigned int width, unsigned int height) {
  /* randomize rand() */
  std::srand(static_cast<int>(time(nullptr)));

  /* init app mode map first */
  mRenderData.mAppModeMap[appMode::edit] = "Edit";
  mRenderData.mAppModeMap[appMode::view] = "View";

  /* save orig window title, add current mode */
  mOrigWindowTitle = mModelInstCamData.micGetWindowTitleFunction();
  setModeInWindowTitle();

  /* save orig window title, add current mode */
  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  if (!mRenderData.rdWindow) {
    Logger::log(1, "%s error: invalid GLFWwindow handle\n", __FUNCTION__);
    return false;
  }

  if (!deviceInit()) {
    return false;
  }

  if (!initVma()) {
    return false;
  }

  if (!getQueues()) {
    return false;
  }

  if (!createSwapchain()) {
    return false;
  }

  /* must be done AFTER swapchain as we need data from it */
  if (!createDepthBuffer()) {
    return false;
  }

  if (!createSelectionImage()) {
    return false;
  }

  if (!createCommandPools()) {
    return false;
  }

  if (!createCommandBuffers()) {
    return false;
  }

  if (!createVertexBuffers()) {
    return false;
  }

  if (!createMatrixUBO()) {
    return false;
  }

  if (!createSSBOs()) {
    return false;
  }

  if (!createDescriptorPool()) {
    return false;
  }

  if (!createDescriptorLayouts()) {
    return false;
  }

  if (!createDescriptorSets()) {
    return false;
  }

  if (!createRenderPass()) {
    return false;
  }

  if (!createPipelineLayouts()) {
    return false;
  }

  if (!createPipelines()) {
    return false;
  }

  if (!createFramebuffer()) {
    return false;
  }

  if (!createSyncObjects()) {
    return false;
  }

  if (!initUserInterface()) {
    return false;
  }

  /* init quadtree with some default values */
  mWorldBoundaries = std::make_shared<BoundingBox2D>(mRenderData.rdWorldStartPos, mRenderData.rdWorldSize);
  initQuadTree(10, 5);
  Logger::log(1, "%s: quadtree initialized\n", __FUNCTION__);

  mModelInstCamData.micQuadTreeFindAllIntersectionsCallbackFunction = [this]() { return mQuadtree->findAllIntersections(); };
  mModelInstCamData.micQuadTreeGetBoxesCallbackFunction = [this]() { return mQuadtree->getTreeBoxes(); };
  mModelInstCamData.micWorldGetBoundariesCallbackFunction = [this]() { return getWorldBoundaries(); };

  /* register instance/model callbacks */
  mModelInstCamData.micModelCheckCallbackFunction = [this](std::string fileName) { return hasModel(fileName); };
  mModelInstCamData.micModelAddCallbackFunction = [this](std::string fileName, bool initialInstance, bool withUndo) { return addModel(fileName, initialInstance, withUndo); };
  mModelInstCamData.micModelDeleteCallbackFunction = [this](std::string modelName, bool withUndo) { deleteModel(modelName, withUndo); };

  mModelInstCamData.micInstanceAddCallbackFunction = [this](std::shared_ptr<AssimpModel> model) { return addInstance(model); };
  mModelInstCamData.micInstanceAddManyCallbackFunction = [this](std::shared_ptr<AssimpModel> model, int numInstances) { addInstances(model, numInstances); };
  mModelInstCamData.micInstanceDeleteCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance, bool withUndo) { deleteInstance(instance, withUndo) ;};
  mModelInstCamData.micInstanceCloneCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { cloneInstance(instance); };
  mModelInstCamData.micInstanceCloneManyCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance, int numClones) { cloneInstances(instance, numClones); };

  mModelInstCamData.micInstanceCenterCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { centerInstance(instance); };

  mModelInstCamData.micUndoCallbackFunction = [this]() { undoLastOperation(); };
  mModelInstCamData.micRedoCallbackFunction = [this]() { redoLastOperation(); };

  mModelInstCamData.micLoadConfigCallbackFunction = [this](std::string configFileName) { return loadConfigFile(configFileName); };
  mModelInstCamData.micSaveConfigCallbackFunction = [this](std::string configFileName) { return saveConfigFile(configFileName); };
  mModelInstCamData.micNewConfigCallbackFunction = [this]() { createEmptyConfig(); };

  mModelInstCamData.micSetConfigDirtyCallbackFunction = [this](bool flag) { setConfigDirtyFlag(flag); };
  mModelInstCamData.micGetConfigDirtyCallbackFunction = [this]() { return getConfigDirtyFlag(); };

  mModelInstCamData.micCameraCloneCallbackFunction = [this]() { cloneCamera(); };
  mModelInstCamData.micCameraDeleteCallbackFunction = [this]() { deleteCamera(); };
  mModelInstCamData.micCameraNameCheckCallbackFunction = [this](std::string cameraName) { return checkCameraNameUsed(cameraName); };

  mModelInstCamData.micInstanceGetPositionsCallbackFunction = [this]() { return get2DPositionOfAllInstances(); };
  mModelInstCamData.micQuadTreeQueryBBoxCallbackFunction = [this](BoundingBox2D box) { return mQuadtree->query(box); };

  mModelInstCamData.micEditNodeGraphCallbackFunction = [this](std::string graphName) { editGraph(graphName); };
  mModelInstCamData.micCreateEmptyNodeGraphCallbackFunction= [this]() { return createEmptyGraph(); };

  mModelInstCamData.micInstanceAddBehaviorCallbackFunction = [this](int instanceId, std::shared_ptr<SingleInstanceBehavior> behavior) {
    addBehavior(instanceId, behavior);
  };
  mModelInstCamData.micInstanceDelBehaviorCallbackFunction = [this](int instanceId) { delBehavior(instanceId); };
  mModelInstCamData.micModelAddBehaviorCallbackFunction = [this](std::string modelName, std::shared_ptr<SingleInstanceBehavior> behavior) {
    addModelBehavior(modelName, behavior);
  };
  mModelInstCamData.micModelDelBehaviorCallbackFunction = [this](std::string modelName) { delModelBehavior(modelName); };
  mModelInstCamData.micNodeEventCallbackFunction = [this](int instanceId, nodeEvent event) { addBehaviorEvent(instanceId, event); };
  mModelInstCamData.micPostNodeTreeDelBehaviorCallbackFunction = [this](std::string nodeTreeName) { postDelNodeTree(nodeTreeName); };

  mRenderData.rdAppExitCallbackFunction = [this]() { doExitApplication(); };
  Logger::log(1, "%s: callbacks initialized\n", __FUNCTION__);

  /* init camera strings */
  mModelInstCamData.micCameraProjectionMap[cameraProjection::perspective] = "Perspective";
  mModelInstCamData.micCameraProjectionMap[cameraProjection::orthogonal] = "Orthogonal";

  mModelInstCamData.micCameraTypeMap[cameraType::free] = "Free";
  mModelInstCamData.micCameraTypeMap[cameraType::firstPerson] = "First Person";
  mModelInstCamData.micCameraTypeMap[cameraType::thirdPerson] = "Third Person";
  mModelInstCamData.micCameraTypeMap[cameraType::stationary] = "Stationary (fixed)";
  mModelInstCamData.micCameraTypeMap[cameraType::stationaryFollowing] = "Stationary (following target)";

  /* init other maps */
  mModelInstCamData.micMoveStateMap[moveState::idle] = "Idle";
  mModelInstCamData.micMoveStateMap[moveState::walk] = "Walk";
  mModelInstCamData.micMoveStateMap[moveState::run] = "Run";
  mModelInstCamData.micMoveStateMap[moveState::jump] = "Jump";
  mModelInstCamData.micMoveStateMap[moveState::hop] = "Hop";
  mModelInstCamData.micMoveStateMap[moveState::pick] = "Pick";
  mModelInstCamData.micMoveStateMap[moveState::punch] = "Punch";
  mModelInstCamData.micMoveStateMap[moveState::roll] = "Roll";
  mModelInstCamData.micMoveStateMap[moveState::kick] = "Kick";
  mModelInstCamData.micMoveStateMap[moveState::interact] = "Interact";
  mModelInstCamData.micMoveStateMap[moveState::wave] = "Wave";

  mModelInstCamData.micMoveDirectionMap[moveDirection::none] = "None";
  mModelInstCamData.micMoveDirectionMap[moveDirection::forward] = "Forward";
  mModelInstCamData.micMoveDirectionMap[moveDirection::back] = "Backward";
  mModelInstCamData.micMoveDirectionMap[moveDirection::left] = "Left";
  mModelInstCamData.micMoveDirectionMap[moveDirection::right] = "Right";
  mModelInstCamData.micMoveDirectionMap[moveDirection::any] = "Any";

  mModelInstCamData.micNodeUpdateMap[nodeEvent::none] = "None";
  mModelInstCamData.micNodeUpdateMap[nodeEvent::instanceToInstanceCollision] = "Inst to Inst collision";
  mModelInstCamData.micNodeUpdateMap[nodeEvent::instanceToEdgeCollision] = "Inst to Edge collision";
  mModelInstCamData.micNodeUpdateMap[nodeEvent::interaction] = "Interaction";

  mModelInstCamData.micFaceAnimationNameMap[faceAnimation::none] = "None";
  mModelInstCamData.micFaceAnimationNameMap[faceAnimation::angry] = "Angry";
  mModelInstCamData.micFaceAnimationNameMap[faceAnimation::worried] = "Worried";
  mModelInstCamData.micFaceAnimationNameMap[faceAnimation::surprised] = "Surprised";
  mModelInstCamData.micFaceAnimationNameMap[faceAnimation::happy] = "Happy";

  mModelInstCamData.micHeadMoveAnimationNameMap[headMoveDirection::left] = "Left";
  mModelInstCamData.micHeadMoveAnimationNameMap[headMoveDirection::right] = "Right";
  mModelInstCamData.micHeadMoveAnimationNameMap[headMoveDirection::up] = "Up";
  mModelInstCamData.micHeadMoveAnimationNameMap[headMoveDirection::down] = "Down";

  Logger::log(1, "%s: enum to string maps initialized\n", __FUNCTION__);

  /* valid, but emtpy line mesh */
  mLineMesh = std::make_shared<VkLineMesh>();
  Logger::log(1, "%s: line mesh storage initialized\n", __FUNCTION__);

  mAABBMesh = std::make_shared<VkLineMesh>();
  Logger::log(1, "%s: AABB line mesh storage initialized\n", __FUNCTION__);

  mSphereModel = SphereModel(1.0, 5, 8, glm::vec3(1.0f, 1.0f, 1.0f));
  mSphereMesh = mSphereModel.getVertexData();
  Logger::log(1, "%s: Sphere line mesh storage initialized\n", __FUNCTION__);

  mCollidingSphereModel = SphereModel(1.0, 5, 8, glm::vec3(1.0f, 0.0f, 0.0f));
  mCollidingSphereMesh = mCollidingSphereModel.getVertexData();
  Logger::log(1, "%s: Colliding sphere line mesh storage initialized\n", __FUNCTION__);

  mBehavior = std::make_shared<Behavior>();
  mInstanceNodeActionCallbackFunction = [this](int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    updateInstanceSettings(instanceId, nodeType, updateType, data, extraSetting);
  };
  mBehavior->setNodeActionCallback(mInstanceNodeActionCallbackFunction);
  Logger::log(1, "%s: behavior data initialized\n", __FUNCTION__);

  mGraphEditor = std::make_shared<GraphEditor>();
  Logger::log(1, "%s: graph editor initialized\n", __FUNCTION__);

  /* signal graphics semaphore before doing anything else to be able to run compute submit */
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &mRenderData.rdGraphicSemaphore;

  VkResult result = vkQueueSubmit(mRenderData.rdGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to submit initial semaphore (%i)\n", __FUNCTION__, result);
    return false;
  }

  /* try to load the default configuration file */
  if (loadConfigFile(mDefaultConfigFileName)) {
    Logger::log(1, "%s: loaded default config file '%s'\n", __FUNCTION__, mDefaultConfigFileName.c_str());
  } else {
    Logger::log(1, "%s: could not load default config file '%s'\n", __FUNCTION__, mDefaultConfigFileName.c_str());
    /* clear everything and add null model/instance/settings container */
    createEmptyConfig();
  }

  mFrameTimer.start();

  Logger::log(1, "%s: Vulkan renderer initialized to %ix%i\n", __FUNCTION__, width, height);

  mApplicationRunning = true;
  return true;
}

ModelInstanceCamData& VkRenderer::getModInstCamData() {
  return mModelInstCamData;
}

bool VkRenderer::loadConfigFile(std::string configFileName) {
  YamlParser parser;
  if (!parser.loadYamlFile(configFileName)) {
    return false;
  }

  std::string yamlFileVersion = parser.getFileVersion();
  if (yamlFileVersion.empty()) {
    Logger::log(1, "%s error: could not check file version of YAML config file '%s'\n", __FUNCTION__, parser.getFileName().c_str());
    return false;
  }

  /* we delete all models and instances at this point, the requesting dialog has been confirmed */
  removeAllModelsAndInstances();

  /* get models */
  std::vector<ModelSettings> savedModelSettings = parser.getModelConfigs();
  if (savedModelSettings.size() == 0) {
    Logger::log(1, "%s error: no model files in file '%s'\n", __FUNCTION__, parser.getFileName().c_str());
    return false;
  }

  for (auto& modSetting : savedModelSettings) {
    if (!addModel(modSetting.msModelFilenamePath, false, false)) {
      return false;
    }
    std::shared_ptr<AssimpModel> model = getModel(modSetting.msModelFilenamePath);
    if (!model) {
      return false;
    }

    /* migration config version 3.0 to 4.0+  */
    if (yamlFileVersion == "3.0") {
      Logger::log(1, "%s: adding empty bounding sphere adjustment vector\n", __FUNCTION__);
      std::vector<glm::vec4> boundingSphereAdjustments = model->getModelSettings().msBoundingSphereAdjustments;
      modSetting.msBoundingSphereAdjustments = boundingSphereAdjustments;
    }

    model->setModelSettings(modSetting);
  }

  /* restore selected model number */
  int selectedModel = parser.getSelectedModelNum();
  if (selectedModel < mModelInstCamData.micModelList.size()) {
    mModelInstCamData.micSelectedModel = selectedModel;
  } else {
    mModelInstCamData.micSelectedModel = 0;
  }

  /* get node trees for behavior, needed to be set (copied) in instances */
  std::vector<EnhancedBehaviorData> behaviorData = parser.getBehaviorData();
  if (behaviorData.size() == 0) {
    Logger::log(1, "%s error: no behaviors in file '%s'\n", __FUNCTION__, parser.getFileName().c_str());
  }

  for (const auto& behavior : behaviorData) {
    Logger::log(1, "%s: found behavior '%s'\n", __FUNCTION__, behavior.bdName.c_str());

    std::shared_ptr<SingleInstanceBehavior> newBehavior = std::make_shared<SingleInstanceBehavior>();
    std::shared_ptr<GraphNodeFactory> factory = std::make_shared<GraphNodeFactory>([&](int nodeId) { newBehavior->updateNodeStatus(nodeId); });

    std::shared_ptr<BehaviorData> data = newBehavior->getBehaviorData();
    for (const auto& link : behavior.bdGraphLinks) {
      Logger::log(1, "%s: found link %i from out pin %i to in pin %i\n", __FUNCTION__, link.first, link.second.first, link.second.second);
    }
    data->bdGraphLinks = behavior.bdGraphLinks;

    for (const auto& nodeData : behavior.nodeImportData) {
      data->bdGraphNodes.emplace_back(factory->makeNode(nodeData.nodeType, nodeData.nodeId));
      Logger::log(1, "%s: created new node %i with type %i\n", __FUNCTION__, nodeData.nodeId, nodeData.nodeType);

      int newNodeId = nodeData.nodeId;
      const auto iter = std::find_if(data->bdGraphNodes.begin(), data->bdGraphNodes.end(), [newNodeId](const auto& existingNode) {
        return existingNode->getNodeId() == newNodeId;
      });

      for (const auto& prop : nodeData.nodeProperties) {
        Logger::log(1, "%s: %s has prop %s\n", __FUNCTION__, prop.first.c_str(), prop.second.c_str());
      }
      if (iter != data->bdGraphNodes.end()) {
        (*iter)->importData(nodeData.nodeProperties);
      }
    }

    data->bdEditorSettings = behavior.bdEditorSettings;
    data->bdName = behavior.bdName;

    mModelInstCamData.micBehaviorData.emplace(behavior.bdName, std::move(newBehavior));
  }

  /* load instances */
  std::vector<ExtendedInstanceSettings> savedInstanceSettings = parser.getInstanceConfigs();
  if (savedInstanceSettings.size() == 0) {
    Logger::log(1, "%s error: no instance in file '%s'\n", __FUNCTION__, parser.getFileName().c_str());
    return false;
  }

  for (const auto& instSettings : savedInstanceSettings) {
    std::shared_ptr<AssimpInstance> newInstance = addInstance(getModel(instSettings.isModelFile), false);
    newInstance->setInstanceSettings(instSettings);
  }

  enumerateInstances();

  /* restore selected instance num */
  int selectedInstance = parser.getSelectedInstanceNum();
  if (selectedInstance < mModelInstCamData.micAssimpInstances.size()) {
    mModelInstCamData.micSelectedInstance = selectedInstance;
  } else {
    mModelInstCamData.micSelectedInstance = 0;
  }

  /* restore behavior data after IDs are restored */
  for (auto& instance : mModelInstCamData.micAssimpInstances) {
    InstanceSettings instSettings = instance->getInstanceSettings();
    if (!instSettings.isNodeTreeName.empty()) {
      addBehavior(instSettings.isInstanceIndexPosition, mModelInstCamData.micBehaviorData.at(instSettings.isNodeTreeName));
    }
  }

  /* make sure we have the default cam */
  loadDefaultFreeCam();

  /* load cameras */
  std::vector<CameraSettings> savedCamSettings = parser.getCameraConfigs();
  if (savedCamSettings.size() == 0) {
    Logger::log(1, "%s warning: no cameras in file '%s', fallback to default\n", __FUNCTION__, parser.getFileName().c_str());
  } else {
    for (const auto& setting : savedCamSettings) {
      /* camera instance zero is always available, just import settings */
      if (setting.csCamName == "FreeCam") {
        Logger::log(1, "%s: restore FreeCam\n", __FUNCTION__);
        mModelInstCamData.micCameras.at(0)->setCameraSettings(setting);
      } else {
        Logger::log(1, "%s: restore camera %s\n", __FUNCTION__, setting.csCamName.c_str());
        std::shared_ptr<Camera> newCam = std::make_shared<Camera>();
        newCam->setCameraSettings(setting);
        mModelInstCamData.micCameras.emplace_back(newCam);
      }
    }

    /* now try to set the camera targets back to the chosen instances */
    for (int i = 0; i < savedInstanceSettings.size(); ++i) {
      if (!savedInstanceSettings.at(i).eisCameraNames.empty()) {
        for (const auto& camName : savedInstanceSettings.at(i).eisCameraNames) {
          /* skip over null instance */
          int instanceId = i + 1;

          /* double check */
          if (instanceId < mModelInstCamData.micAssimpInstances.size()) {
            Logger::log(1, "%s: restore camera instance settings for instance %i (cam: %s)\n", __FUNCTION__, instanceId, camName.c_str());
            std::shared_ptr<AssimpInstance> instanceToFollow = mModelInstCamData.micAssimpInstances.at(instanceId);

            auto iter = std::find_if(mModelInstCamData.micCameras.begin(), mModelInstCamData.micCameras.end(), [camName](std::shared_ptr<Camera> cam) {
              return cam->getCameraSettings().csCamName == camName;
            });
            if (iter != mModelInstCamData.micCameras.end()) {
              (*iter)->setInstanceToFollow(instanceToFollow);
            }
          }
        }
      }
    }

    /* restore selected camera num */
    int selectedCamera = parser.getSelectedCameraNum();
    if (selectedCamera < mModelInstCamData.micCameras.size()) {
      mModelInstCamData.micSelectedCamera = selectedCamera;
    } else {
      mModelInstCamData.micSelectedCamera = 0;
    }
  }

  /* restore hightlight status, set default edit mode */
  mRenderData.rdHighlightSelectedInstance = parser.getHighlightActivated();
  mRenderData.rdInstanceEditMode = instanceEditMode::move;

  /* restore collision and interaction settings */
  mRenderData.rdCheckCollisions = parser.getCollisionChecksEnabled();
  mRenderData.rdInteraction = parser.getInteractionEnabled();
  mRenderData.rdInteractionMinRange = parser.getInteractionMinRange();
  mRenderData.rdInteractionMaxRange = parser.getInteractionMaxRange();
  mRenderData.rdInteractionFOV = parser.getInteractionFOV();

  return true;
}

bool VkRenderer::saveConfigFile(std::string configFileName) {
  if (mModelInstCamData.micAssimpInstancesPerModel.size() == 1) {
    Logger::log(1, "%s error: nothing to save (no models)\n", __FUNCTION__);
    return false;
  }

  YamlParser parser;
  if (!parser.createConfigFile(mRenderData, mModelInstCamData)) {
    Logger::log(1, "%s error: could not create YAML config file!\n", __FUNCTION__);
    return false;
  }

  return parser.writeYamlFile(configFileName);
}

void VkRenderer::createEmptyConfig() {
  removeAllModelsAndInstances();
  loadDefaultFreeCam();
}

void VkRenderer::requestExitApplication() {
  /* set app mode back to edit to show windows */
  mRenderData.rdApplicationMode = appMode::edit;
  mRenderData.rdRequestApplicationExit = true;
}

void VkRenderer::doExitApplication() {
  mApplicationRunning = false;
}

void VkRenderer::undoLastOperation() {
  if (mModelInstCamData.micSettingsContainer->getUndoSize() == 0) {
    return;
  }

  mModelInstCamData.micSettingsContainer->undo();
  /* we need to update the index numbers in case instances were deleted,
   * and the settings files still contain the old index number */
  enumerateInstances();

  int selectedInstance = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  if (selectedInstance < mModelInstCamData.micAssimpInstances.size()) {
    mModelInstCamData.micSelectedInstance = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  } else {
    mModelInstCamData.micSelectedInstance = 0;
  }

  /* if we made all changes undone, the config is no longer dirty */
  if (mModelInstCamData.micSettingsContainer->getUndoSize() == 0) {
    setConfigDirtyFlag(false);
  }
}

void VkRenderer::redoLastOperation() {
  if (mModelInstCamData.micSettingsContainer->getRedoSize() == 0) {
    return;
  }

  mModelInstCamData.micSettingsContainer->redo();
  enumerateInstances();

  int selectedInstance = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  if (selectedInstance < mModelInstCamData.micAssimpInstances.size()) {
    mModelInstCamData.micSelectedInstance = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  } else {
    mModelInstCamData.micSelectedInstance = 0;
  }

  /* if any changes have been re-done, the config is dirty */
  if (mModelInstCamData.micSettingsContainer->getUndoSize() > 0) {
    setConfigDirtyFlag(true);
  }
}

void VkRenderer::addNullModelAndInstance() {
  /* create an empty null model and an instance from it */
  std::shared_ptr<AssimpModel> nullModel = std::make_shared<AssimpModel>();
  mModelInstCamData.micModelList.emplace_back(nullModel);

  std::shared_ptr<AssimpInstance> nullInstance = std::make_shared<AssimpInstance>(nullModel);
  mModelInstCamData.micAssimpInstancesPerModel[nullModel->getModelFileName()].emplace_back(nullInstance);
  mModelInstCamData.micAssimpInstances.emplace_back(nullInstance);
  enumerateInstances();

  /* init the central settings container */
  mModelInstCamData.micSettingsContainer.reset();
  mModelInstCamData.micSettingsContainer = std::make_shared<AssimpSettingsContainer>(nullInstance);
}

void VkRenderer::createSettingsContainerCallbacks() {
  mModelInstCamData.micSettingsContainer->getSelectedModelCallbackFunction = [this]() {return mModelInstCamData.micSelectedModel; };
  mModelInstCamData.micSettingsContainer->setSelectedModelCallbackFunction = [this](int modelId) { mModelInstCamData.micSelectedModel = modelId; };

  mModelInstCamData.micSettingsContainer->modelDeleteCallbackFunction = [this](std::string modelFileName, bool withUndo) { deleteModel(modelFileName, withUndo); };
  mModelInstCamData.micSettingsContainer->modelAddCallbackFunction = [this](std::string modelFileName, bool initialInstance, bool withUndo) { return addModel(modelFileName, initialInstance, withUndo); };
  mModelInstCamData.micSettingsContainer->modelAddExistingCallbackFunction = [this](std::shared_ptr<AssimpModel> model, int indexPos) { addExistingModel(model, indexPos); };


  mModelInstCamData.micSettingsContainer->getSelectedInstanceCallbackFunction = [this]() { return mModelInstCamData.micSelectedInstance; };
  mModelInstCamData.micSettingsContainer->setSelectedInstanceCallbackFunction = [this](int instanceId) { mModelInstCamData.micSelectedInstance = instanceId; };

  mModelInstCamData.micSettingsContainer->getInstanceEditModeCallbackFunction = [this]() { return mRenderData.rdInstanceEditMode; };
  mModelInstCamData.micSettingsContainer->setInstanceEditModeCallbackFunction = [this](instanceEditMode mode) { mRenderData.rdInstanceEditMode = mode; };

  mModelInstCamData.micSettingsContainer->instanceGetModelCallbackFunction = [this](std::string fileName) { return getModel(fileName); };
  mModelInstCamData.micSettingsContainer->instanceAddCallbackFunction = [this](std::shared_ptr<AssimpModel> model) { return addInstance(model); };
  mModelInstCamData.micSettingsContainer->instanceAddExistingCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance, int indexPos, int indexPerModelPos)
    { addExistingInstance(instance, indexPos, indexPerModelPos); };
  mModelInstCamData.micSettingsContainer->instanceDeleteCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance, bool withUndo) { deleteInstance(instance, withUndo) ;};
}

void VkRenderer::clearUndoRedoStacks() {
  mModelInstCamData.micSettingsContainer->removeStacks();
}

void VkRenderer::removeAllModelsAndInstances() {
  mModelInstCamData.micSelectedInstance = 0;
  mModelInstCamData.micSelectedModel = 0;

  mModelInstCamData.micAssimpInstances.erase(mModelInstCamData.micAssimpInstances.begin(),mModelInstCamData.micAssimpInstances.end());
  mModelInstCamData.micAssimpInstancesPerModel.clear();

  /* cleanup remaining models */
  for (auto& model : mModelInstCamData.micModelList) {
    mModelInstCamData.micPendingDeleteAssimpModels.insert(model);
  }
  mModelInstCamData.micDoDeletePendingAssimpModels = true;

  mModelInstCamData.micModelList.erase(mModelInstCamData.micModelList.begin(), mModelInstCamData.micModelList.end());

  /* reset behavior data and graphEditor */
  mBehavior->clear();
  mModelInstCamData.micBehaviorData.clear();
  mGraphEditor = std::make_shared<GraphEditor>();

  /* no instances, no dirty flag (catches 'load' and 'new') */
  setConfigDirtyFlag(false);

  /* re-add null model and instance */
  addNullModelAndInstance();

  /* add callbacks */
  createSettingsContainerCallbacks();

  /* kill undo and redo stacks too */
  clearUndoRedoStacks();

  /* reset collision settings */
  resetCollisionData();

  updateTriangleCount();
}

void VkRenderer::resetCollisionData() {
  mModelInstCamData.micInstanceCollisions.clear();

  mRenderData.rdNumberOfCollisions = 0;
  mRenderData.rdCheckCollisions = collisionChecks::none;
  mRenderData.rdDrawCollisionAABBs = collisionDebugDraw::none;
  mRenderData.rdDrawBoundingSpheres = collisionDebugDraw::none;
}

void VkRenderer::loadDefaultFreeCam() {
  mModelInstCamData.micCameras.clear();

  std::shared_ptr<Camera> freeCam = std::make_shared<Camera>();
  freeCam->setName("FreeCam");
  mModelInstCamData.micCameras.emplace_back(freeCam);

  mModelInstCamData.micSelectedCamera = 0;
}

bool VkRenderer::deviceInit() {
  /* instance and window - we need at least Vukan 1.1 for the "VK_KHR_maintenance1" extension */
  vkb::InstanceBuilder instBuild;
  auto instRet = instBuild
    .use_default_debug_messenger()
    .request_validation_layers()
    .require_api_version(1, 1, 0)
    .build();

  if (!instRet) {
    Logger::log(1, "%s error: could not build vkb instance\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdVkbInstance = instRet.value();

  VkResult result = glfwCreateWindowSurface(mRenderData.rdVkbInstance, mRenderData.rdWindow, nullptr, &mSurface);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: Could not create Vulkan surface (error: %i)\n", __FUNCTION__);
    return false;
  }

  /* force anisotropy */
  VkPhysicalDeviceFeatures requiredFeatures{};
  requiredFeatures.samplerAnisotropy = VK_TRUE;

  /* just get the first available device */
  vkb::PhysicalDeviceSelector physicalDevSel{mRenderData.rdVkbInstance};
  auto firstPysicalDevSelRet = physicalDevSel
    .set_surface(mSurface)
    .set_required_features(requiredFeatures)
    .select();

  if (!firstPysicalDevSelRet) {
    Logger::log(1, "%s error: could not get physical devices\n", __FUNCTION__);
    return false;
  }

  /* a 2nd call is required to enable all the supported features, like wideLines */
  VkPhysicalDeviceFeatures physFeatures;
  vkGetPhysicalDeviceFeatures(firstPysicalDevSelRet.value(), &physFeatures);

  auto secondPhysicalDevSelRet = physicalDevSel
    .set_surface(mSurface)
    .set_required_features(physFeatures)
    .select();

  if (!secondPhysicalDevSelRet) {
    Logger::log(1, "%s error: could not get physical devices\n", __FUNCTION__);
    return false;
  }

  mRenderData.rdVkbPhysicalDevice = secondPhysicalDevSelRet.value();
  Logger::log(1, "%s: found physical device '%s'\n", __FUNCTION__, mRenderData.rdVkbPhysicalDevice.name.c_str());

  /* required for dynamic buffer with world position matrices */
  VkDeviceSize minSSBOOffsetAlignment = mRenderData.rdVkbPhysicalDevice.properties.limits.minStorageBufferOffsetAlignment;
  Logger::log(1, "%s: the physical device has a minimal SSBO offset of %i bytes\n", __FUNCTION__, minSSBOOffsetAlignment);
  mMinSSBOOffsetAlignment = std::max(minSSBOOffsetAlignment, sizeof(glm::mat4));
  Logger::log(1, "%s: SSBO offset has been adjusted to %i bytes\n", __FUNCTION__, mMinSSBOOffsetAlignment);

  vkb::DeviceBuilder devBuilder{mRenderData.rdVkbPhysicalDevice};
  auto devBuilderRet = devBuilder.build();
  if (!devBuilderRet) {
    Logger::log(1, "%s error: could not get devices\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdVkbDevice = devBuilderRet.value();

  return true;
}

bool VkRenderer::getQueues() {
  auto graphQueueRet = mRenderData.rdVkbDevice.get_queue(vkb::QueueType::graphics);
  if (!graphQueueRet.has_value()) {
    Logger::log(1, "%s error: could not get graphics queue\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdGraphicsQueue = graphQueueRet.value();

  auto presentQueueRet = mRenderData.rdVkbDevice.get_queue(vkb::QueueType::present);
  if (!presentQueueRet.has_value()) {
    Logger::log(1, "%s error: could not get present queue\n", __FUNCTION__);
    return false;
  }
  mRenderData.rdPresentQueue = presentQueueRet.value();

  auto computeQueueRet = mRenderData.rdVkbDevice.get_queue(vkb::QueueType::compute);
  if (!computeQueueRet.has_value()) {
    Logger::log(1, "%s: using shared graphics/compute queue\n", __FUNCTION__);
    mRenderData.rdComputeQueue = mRenderData.rdGraphicsQueue;
    mHasDedicatedComputeQueue = false;
  } else {
    Logger::log(1, "%s: using separate compute queue\n", __FUNCTION__);
    mRenderData.rdComputeQueue = computeQueueRet.value();
    mHasDedicatedComputeQueue = true;
  }

  return true;
}

bool VkRenderer::createDescriptorPool() {
  std::vector<VkDescriptorPoolSize> poolSizes =
  {
    { VK_DESCRIPTOR_TYPE_SAMPLER, 10000 },
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000 },
    { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10000 },
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
  };

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.maxSets = 10000;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();

  VkResult result = vkCreateDescriptorPool(mRenderData.rdVkbDevice.device, &poolInfo, nullptr, &mRenderData.rdDescriptorPool);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not init descriptor pool (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  return true;
}

bool VkRenderer::createDescriptorLayouts() {
  VkResult result;

  {
    /* texture */
    VkDescriptorSetLayoutBinding assimpTextureBind{};
    assimpTextureBind.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    assimpTextureBind.binding = 0;
    assimpTextureBind.descriptorCount = 1;
    assimpTextureBind.pImmutableSamplers = nullptr;
    assimpTextureBind.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpTexBindings = { assimpTextureBind };

    VkDescriptorSetLayoutCreateInfo assimpTextureCreateInfo{};
    assimpTextureCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpTextureCreateInfo.bindingCount = static_cast<uint32_t>(assimpTexBindings.size());
    assimpTextureCreateInfo.pBindings = assimpTexBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpTextureCreateInfo,
      nullptr, &mRenderData.rdAssimpTextureDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp texture descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* non-animated shader */
    VkDescriptorSetLayoutBinding assimpUboBind{};
    assimpUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpUboBind.binding = 0;
    assimpUboBind.descriptorCount = 1;
    assimpUboBind.pImmutableSamplers = nullptr;
    assimpUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSsboBind{};
    assimpSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSsboBind.binding = 1;
    assimpSsboBind.descriptorCount = 1;
    assimpSsboBind.pImmutableSamplers = nullptr;
    assimpSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSsboBind2{};
    assimpSsboBind2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSsboBind2.binding = 2;
    assimpSsboBind2.descriptorCount = 1;
    assimpSsboBind2.pImmutableSamplers = nullptr;
    assimpSsboBind2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpBindings = { assimpUboBind, assimpSsboBind, assimpSsboBind2 };

    VkDescriptorSetLayoutCreateInfo assimpCreateInfo{};
    assimpCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpCreateInfo.bindingCount = static_cast<uint32_t>(assimpBindings.size());
    assimpCreateInfo.pBindings = assimpBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpCreateInfo,
      nullptr, &mRenderData.rdAssimpDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* animated shader */
    VkDescriptorSetLayoutBinding assimpUboBind{};
    assimpUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpUboBind.binding = 0;
    assimpUboBind.descriptorCount = 1;
    assimpUboBind.pImmutableSamplers = nullptr;
    assimpUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSsboBind{};
    assimpSkinningSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSsboBind.binding = 1;
    assimpSkinningSsboBind.descriptorCount = 1;
    assimpSkinningSsboBind.pImmutableSamplers = nullptr;
    assimpSkinningSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSsboBind2{};
    assimpSkinningSsboBind2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSsboBind2.binding = 2;
    assimpSkinningSsboBind2.descriptorCount = 1;
    assimpSkinningSsboBind2.pImmutableSamplers = nullptr;
    assimpSkinningSsboBind2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSsboBind3{};
    assimpSkinningSsboBind3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSsboBind3.binding = 3;
    assimpSkinningSsboBind3.descriptorCount = 1;
    assimpSkinningSsboBind3.pImmutableSamplers = nullptr;
    assimpSkinningSsboBind3.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpSkinningBindings =
      { assimpUboBind, assimpSkinningSsboBind, assimpSkinningSsboBind2, assimpSkinningSsboBind3 };

    VkDescriptorSetLayoutCreateInfo assimpSkinningCreateInfo{};
    assimpSkinningCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpSkinningCreateInfo.bindingCount = static_cast<uint32_t>(assimpSkinningBindings.size());
    assimpSkinningCreateInfo.pBindings = assimpSkinningBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpSkinningCreateInfo,
      nullptr, &mRenderData.rdAssimpSkinningDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp skinning buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* non-animated selection shader */
    VkDescriptorSetLayoutBinding assimpSelUboBind{};
    assimpSelUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpSelUboBind.binding = 0;
    assimpSelUboBind.descriptorCount = 1;
    assimpSelUboBind.pImmutableSamplers = nullptr;
    assimpSelUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSelSsboBind{};
    assimpSelSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSelSsboBind.binding = 1;
    assimpSelSsboBind.descriptorCount = 1;
    assimpSelSsboBind.pImmutableSamplers = nullptr;
    assimpSelSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSelSsboBind2{};
    assimpSelSsboBind2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSelSsboBind2.binding = 2;
    assimpSelSsboBind2.descriptorCount = 1;
    assimpSelSsboBind2.pImmutableSamplers = nullptr;
    assimpSelSsboBind2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpBindings = { assimpSelUboBind, assimpSelSsboBind, assimpSelSsboBind2 };

    VkDescriptorSetLayoutCreateInfo assimpSelCreateInfo{};
    assimpSelCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpSelCreateInfo.bindingCount = static_cast<uint32_t>(assimpBindings.size());
    assimpSelCreateInfo.pBindings = assimpBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpSelCreateInfo,
      nullptr, &mRenderData.rdAssimpSelectionDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp selection buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* animated selection shader */
    VkDescriptorSetLayoutBinding assimpSelUboBind{};
    assimpSelUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpSelUboBind.binding = 0;
    assimpSelUboBind.descriptorCount = 1;
    assimpSelUboBind.pImmutableSamplers = nullptr;
    assimpSelUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSelSsboBind{};
    assimpSkinningSelSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSelSsboBind.binding = 1;
    assimpSkinningSelSsboBind.descriptorCount = 1;
    assimpSkinningSelSsboBind.pImmutableSamplers = nullptr;
    assimpSkinningSelSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSelSsboBind2{};
    assimpSkinningSelSsboBind2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSelSsboBind2.binding = 2;
    assimpSkinningSelSsboBind2.descriptorCount = 1;
    assimpSkinningSelSsboBind2.pImmutableSamplers = nullptr;
    assimpSkinningSelSsboBind2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSelSsboBind3{};
    assimpSkinningSelSsboBind3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSelSsboBind3.binding = 3;
    assimpSkinningSelSsboBind3.descriptorCount = 1;
    assimpSkinningSelSsboBind3.pImmutableSamplers = nullptr;
    assimpSkinningSelSsboBind3.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpSkinningBindings =
      { assimpSelUboBind, assimpSkinningSelSsboBind, assimpSkinningSelSsboBind2, assimpSkinningSelSsboBind3 };

    VkDescriptorSetLayoutCreateInfo assimpSkinningCreateInfo{};
    assimpSkinningCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpSkinningCreateInfo.bindingCount = static_cast<uint32_t>(assimpSkinningBindings.size());
    assimpSkinningCreateInfo.pBindings = assimpSkinningBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpSkinningCreateInfo,
      nullptr, &mRenderData.rdAssimpSkinningSelectionDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp skinning selection buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* animated shader with morphs */
    VkDescriptorSetLayoutBinding assimpUboBind{};
    assimpUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpUboBind.binding = 0;
    assimpUboBind.descriptorCount = 1;
    assimpUboBind.pImmutableSamplers = nullptr;
    assimpUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSsboBind{};
    assimpSkinningSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSsboBind.binding = 1;
    assimpSkinningSsboBind.descriptorCount = 1;
    assimpSkinningSsboBind.pImmutableSamplers = nullptr;
    assimpSkinningSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSsboBind2{};
    assimpSkinningSsboBind2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSsboBind2.binding = 2;
    assimpSkinningSsboBind2.descriptorCount = 1;
    assimpSkinningSsboBind2.pImmutableSamplers = nullptr;
    assimpSkinningSsboBind2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSsboBind3{};
    assimpSkinningSsboBind3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSsboBind3.binding = 3;
    assimpSkinningSsboBind3.descriptorCount = 1;
    assimpSkinningSsboBind3.pImmutableSamplers = nullptr;
    assimpSkinningSsboBind3.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSsboBind4{};
    assimpSkinningSsboBind4.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSsboBind4.binding = 4;
    assimpSkinningSsboBind4.descriptorCount = 1;
    assimpSkinningSsboBind4.pImmutableSamplers = nullptr;
    assimpSkinningSsboBind4.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpSkinningBindings =
      { assimpUboBind, assimpSkinningSsboBind,
        assimpSkinningSsboBind2, assimpSkinningSsboBind3, assimpSkinningSsboBind4 };

    VkDescriptorSetLayoutCreateInfo assimpMorphSkinningCreateInfo{};
    assimpMorphSkinningCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMorphSkinningCreateInfo.bindingCount = static_cast<uint32_t>(assimpSkinningBindings.size());
    assimpMorphSkinningCreateInfo.pBindings = assimpSkinningBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMorphSkinningCreateInfo,
      nullptr, &mRenderData.rdAssimpSkinningMorphDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp morph skinning buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* animated plus morphs selection shader */
    VkDescriptorSetLayoutBinding assimpSelUboBind{};
    assimpSelUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpSelUboBind.binding = 0;
    assimpSelUboBind.descriptorCount = 1;
    assimpSelUboBind.pImmutableSamplers = nullptr;
    assimpSelUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSelSsboBind{};
    assimpSkinningSelSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSelSsboBind.binding = 1;
    assimpSkinningSelSsboBind.descriptorCount = 1;
    assimpSkinningSelSsboBind.pImmutableSamplers = nullptr;
    assimpSkinningSelSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSelSsboBind2{};
    assimpSkinningSelSsboBind2.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSelSsboBind2.binding = 2;
    assimpSkinningSelSsboBind2.descriptorCount = 1;
    assimpSkinningSelSsboBind2.pImmutableSamplers = nullptr;
    assimpSkinningSelSsboBind2.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSelSsboBind3{};
    assimpSkinningSelSsboBind3.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSelSsboBind3.binding = 3;
    assimpSkinningSelSsboBind3.descriptorCount = 1;
    assimpSkinningSelSsboBind3.pImmutableSamplers = nullptr;
    assimpSkinningSelSsboBind3.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpSkinningSelSsboBind4{};
    assimpSkinningSelSsboBind4.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpSkinningSelSsboBind4.binding = 4;
    assimpSkinningSelSsboBind4.descriptorCount = 1;
    assimpSkinningSelSsboBind4.pImmutableSamplers = nullptr;
    assimpSkinningSelSsboBind4.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpSkinningBindings =
    { assimpSelUboBind, assimpSkinningSelSsboBind,
      assimpSkinningSelSsboBind2, assimpSkinningSelSsboBind3, assimpSkinningSelSsboBind4 };

    VkDescriptorSetLayoutCreateInfo assimpMorphSkinningSelectionCreateInfo{};
    assimpMorphSkinningSelectionCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMorphSkinningSelectionCreateInfo.bindingCount = static_cast<uint32_t>(assimpSkinningBindings.size());
    assimpMorphSkinningSelectionCreateInfo.pBindings = assimpSkinningBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMorphSkinningSelectionCreateInfo,
      nullptr, &mRenderData.rdAssimpSkinningMorphSelectionDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp morph skinning selection buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* animated plus morphs, per-model */
    VkDescriptorSetLayoutBinding assimpMorphPerModelSsboBind{};
    assimpMorphPerModelSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpMorphPerModelSsboBind.binding = 0;
    assimpMorphPerModelSsboBind.descriptorCount = 1;
    assimpMorphPerModelSsboBind.pImmutableSamplers = nullptr;
    assimpMorphPerModelSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo assimpMorphPerModelCreateInfo{};
    assimpMorphPerModelCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMorphPerModelCreateInfo.bindingCount = 1;
    assimpMorphPerModelCreateInfo.pBindings = &assimpMorphPerModelSsboBind;

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMorphPerModelCreateInfo,
      nullptr, &mRenderData.rdAssimpSkinningMorphPerModelDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp morph skinning selection per-model buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute transformation shader, global  */
    VkDescriptorSetLayoutBinding assimpTransformSsboBind{};
    assimpTransformSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpTransformSsboBind.binding = 0;
    assimpTransformSsboBind.descriptorCount = 1;
    assimpTransformSsboBind.pImmutableSamplers = nullptr;
    assimpTransformSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding assimpTrsSsboBind{};
    assimpTrsSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpTrsSsboBind.binding = 1;
    assimpTrsSsboBind.descriptorCount = 1;
    assimpTrsSsboBind.pImmutableSamplers = nullptr;
    assimpTrsSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpTransformBindings = { assimpTransformSsboBind, assimpTrsSsboBind };

    VkDescriptorSetLayoutCreateInfo assimpTransformCreateInfo{};
    assimpTransformCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpTransformCreateInfo.bindingCount = static_cast<uint32_t>(assimpTransformBindings.size());
    assimpTransformCreateInfo.pBindings = assimpTransformBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpTransformCreateInfo,
      nullptr, &mRenderData.rdAssimpComputeTransformDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp transform global compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute transformation shader, per-model  */
    VkDescriptorSetLayoutBinding assimpAnimLookupSsboBind{};
    assimpAnimLookupSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpAnimLookupSsboBind.binding = 0;
    assimpAnimLookupSsboBind.descriptorCount = 1;
    assimpAnimLookupSsboBind.pImmutableSamplers = nullptr;
    assimpAnimLookupSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo assimpTransformPerModelCreateInfo{};
    assimpTransformPerModelCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpTransformPerModelCreateInfo.bindingCount = 1;
    assimpTransformPerModelCreateInfo.pBindings = &assimpAnimLookupSsboBind;

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpTransformPerModelCreateInfo,
      nullptr, &mRenderData.rdAssimpComputeTransformPerModelDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp transform per model compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute matrix multiplication shader, global data */
    VkDescriptorSetLayoutBinding assimpTrsSsboBind{};
    assimpTrsSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpTrsSsboBind.binding = 0;
    assimpTrsSsboBind.descriptorCount = 1;
    assimpTrsSsboBind.pImmutableSamplers = nullptr;
    assimpTrsSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding assimpNodeMatricesSsboBind{};
    assimpNodeMatricesSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpNodeMatricesSsboBind.binding = 1;
    assimpNodeMatricesSsboBind.descriptorCount = 1;
    assimpNodeMatricesSsboBind.pImmutableSamplers = nullptr;
    assimpNodeMatricesSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpMatMultBindings =
      { assimpTrsSsboBind,assimpNodeMatricesSsboBind };

    VkDescriptorSetLayoutCreateInfo assimpMatrixMultCreateInfo{};
    assimpMatrixMultCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMatrixMultCreateInfo.bindingCount = static_cast<uint32_t>(assimpMatMultBindings.size());
    assimpMatrixMultCreateInfo.pBindings = assimpMatMultBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMatrixMultCreateInfo,
      nullptr, &mRenderData.rdAssimpComputeMatrixMultDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp matrix multiplication global compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute matrix multiplication shader, per-model data */
    VkDescriptorSetLayoutBinding assimpParentMatrixSsboBind{};
    assimpParentMatrixSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpParentMatrixSsboBind.binding = 0;
    assimpParentMatrixSsboBind.descriptorCount = 1;
    assimpParentMatrixSsboBind.pImmutableSamplers = nullptr;
    assimpParentMatrixSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding assimpBoneOffsetSsboBind{};
    assimpBoneOffsetSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpBoneOffsetSsboBind.binding = 1;
    assimpBoneOffsetSsboBind.descriptorCount = 1;
    assimpBoneOffsetSsboBind.pImmutableSamplers = nullptr;
    assimpBoneOffsetSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding assimpBoundingSpheresSsboBind{};
    assimpBoundingSpheresSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpBoundingSpheresSsboBind.binding = 2;
    assimpBoundingSpheresSsboBind.descriptorCount = 1;
    assimpBoundingSpheresSsboBind.pImmutableSamplers = nullptr;
    assimpBoundingSpheresSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpBoundSpheresPerModelBindings =
      { assimpParentMatrixSsboBind, assimpBoneOffsetSsboBind, assimpBoundingSpheresSsboBind };

    VkDescriptorSetLayoutCreateInfo assimpMatrixMultPerModelCreateInfo{};
    assimpMatrixMultPerModelCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMatrixMultPerModelCreateInfo.bindingCount = static_cast<uint32_t>(assimpBoundSpheresPerModelBindings.size());
    assimpMatrixMultPerModelCreateInfo.pBindings = assimpBoundSpheresPerModelBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMatrixMultPerModelCreateInfo,
      nullptr, &mRenderData.rdAssimpComputeMatrixMultPerModelDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp bounding sphere per model compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute bounding spheres shader, global data */
    VkDescriptorSetLayoutBinding assimpNodeMatrixSsboBind{};
    assimpNodeMatrixSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpNodeMatrixSsboBind.binding = 0;
    assimpNodeMatrixSsboBind.descriptorCount = 1;
    assimpNodeMatrixSsboBind.pImmutableSamplers = nullptr;
    assimpNodeMatrixSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding assimpWorldPosMatricesSsboBind{};
    assimpWorldPosMatricesSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpWorldPosMatricesSsboBind.binding = 1;
    assimpWorldPosMatricesSsboBind.descriptorCount = 1;
    assimpWorldPosMatricesSsboBind.pImmutableSamplers = nullptr;
    assimpWorldPosMatricesSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding boundingSpheresSsboBind{};
    boundingSpheresSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boundingSpheresSsboBind.binding = 2;
    boundingSpheresSsboBind.descriptorCount = 1;
    boundingSpheresSsboBind.pImmutableSamplers = nullptr;
    boundingSpheresSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpMatMultBindings =
    { assimpNodeMatrixSsboBind, assimpWorldPosMatricesSsboBind, boundingSpheresSsboBind };

    VkDescriptorSetLayoutCreateInfo assimpMatrixMultCreateInfo{};
    assimpMatrixMultCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMatrixMultCreateInfo.bindingCount = static_cast<uint32_t>(assimpMatMultBindings.size());
    assimpMatrixMultCreateInfo.pBindings = assimpMatMultBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMatrixMultCreateInfo,
      nullptr, &mRenderData.rdAssimpComputeBoundingSpheresDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp bounding spheres global compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute bounding spheres shader, per-model data */
    VkDescriptorSetLayoutBinding assimpParentMatrixSsboBind{};
    assimpParentMatrixSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpParentMatrixSsboBind.binding = 0;
    assimpParentMatrixSsboBind.descriptorCount = 1;
    assimpParentMatrixSsboBind.pImmutableSamplers = nullptr;
    assimpParentMatrixSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding boundingSphereAdjustmentsSsboBind{};
    boundingSphereAdjustmentsSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boundingSphereAdjustmentsSsboBind.binding = 1;
    boundingSphereAdjustmentsSsboBind.descriptorCount = 1;
    boundingSphereAdjustmentsSsboBind.pImmutableSamplers = nullptr;
    boundingSphereAdjustmentsSsboBind.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpMatMultPerModelBindings =
      { assimpParentMatrixSsboBind, boundingSphereAdjustmentsSsboBind};

    VkDescriptorSetLayoutCreateInfo assimpMatrixMultPerModelCreateInfo{};
    assimpMatrixMultPerModelCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMatrixMultPerModelCreateInfo.bindingCount = static_cast<uint32_t>(assimpMatMultPerModelBindings.size());
    assimpMatrixMultPerModelCreateInfo.pBindings = assimpMatMultPerModelBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMatrixMultPerModelCreateInfo,
      nullptr, &mRenderData.rdAssimpComputeBoundingSpheresPerModelDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp bounding spheres per model compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* line shader */
    VkDescriptorSetLayoutBinding assimpUboBind{};
    assimpUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpUboBind.binding = 0;
    assimpUboBind.descriptorCount = 1;
    assimpUboBind.pImmutableSamplers = nullptr;
    assimpUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpBindings = { assimpUboBind };

    VkDescriptorSetLayoutCreateInfo assimpCreateInfo{};
    assimpCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpCreateInfo.bindingCount = static_cast<uint32_t>(assimpBindings.size());
    assimpCreateInfo.pBindings = assimpBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpCreateInfo,
      nullptr, &mRenderData.rdLineDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp line drawing descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* sphere shader */
    VkDescriptorSetLayoutBinding assimpUboBind{};
    assimpUboBind.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    assimpUboBind.binding = 0;
    assimpUboBind.descriptorCount = 1;
    assimpUboBind.pImmutableSamplers = nullptr;
    assimpUboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding assimpBoundingSpheresSsboBind{};
    assimpBoundingSpheresSsboBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    assimpBoundingSpheresSsboBind.binding = 1;
    assimpBoundingSpheresSsboBind.descriptorCount = 1;
    assimpBoundingSpheresSsboBind.pImmutableSamplers = nullptr;
    assimpBoundingSpheresSsboBind.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::vector<VkDescriptorSetLayoutBinding> assimpSphereBindings =
      { assimpUboBind, assimpBoundingSpheresSsboBind };

    VkDescriptorSetLayoutCreateInfo assimpBoundingSpheresCreateInfo{};
    assimpBoundingSpheresCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpBoundingSpheresCreateInfo.bindingCount = static_cast<uint32_t>(assimpSphereBindings.size());
    assimpBoundingSpheresCreateInfo.pBindings = assimpSphereBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpBoundingSpheresCreateInfo,
      nullptr, &mRenderData.rdSphereDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp bounding sphere drawing descriptor set layout (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  return true;
}

bool VkRenderer::createDescriptorSets() {
  {
    /* non-animated models */
    VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
    descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    descriptorAllocateInfo.descriptorSetCount = 1;
    descriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &descriptorAllocateInfo,
        &mRenderData.rdAssimpDescriptorSet);
     if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* animated models */
    VkDescriptorSetAllocateInfo skinningDescriptorAllocateInfo{};
    skinningDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    skinningDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    skinningDescriptorAllocateInfo.descriptorSetCount = 1;
    skinningDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpSkinningDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &skinningDescriptorAllocateInfo,
      &mRenderData.rdAssimpSkinningDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Skinning descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* selection, non-animated models */
    VkDescriptorSetAllocateInfo selectionDescriptorAllocateInfo{};
    selectionDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    selectionDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    selectionDescriptorAllocateInfo.descriptorSetCount = 1;
    selectionDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpSelectionDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &selectionDescriptorAllocateInfo,
      &mRenderData.rdAssimpSelectionDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp selection descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* selection, animated models */
    VkDescriptorSetAllocateInfo skinningSelectionDescriptorAllocateInfo{};
    skinningSelectionDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    skinningSelectionDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    skinningSelectionDescriptorAllocateInfo.descriptorSetCount = 1;
    skinningSelectionDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpSkinningSelectionDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &skinningSelectionDescriptorAllocateInfo,
      &mRenderData.rdAssimpSkinningSelectionDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp skinning selection descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* animated and morphed models */
    VkDescriptorSetAllocateInfo skinningMorphDescriptorAllocateInfo{};
    skinningMorphDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    skinningMorphDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    skinningMorphDescriptorAllocateInfo.descriptorSetCount = 1;
    skinningMorphDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpSkinningMorphDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &skinningMorphDescriptorAllocateInfo,
      &mRenderData.rdAssimpSkinningMorphDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp morph skinning descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* selection, animated and morphed models */
    VkDescriptorSetAllocateInfo skinningMorphSelectionDescriptorAllocateInfo{};
    skinningMorphSelectionDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    skinningMorphSelectionDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    skinningMorphSelectionDescriptorAllocateInfo.descriptorSetCount = 1;
    skinningMorphSelectionDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpSkinningMorphSelectionDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &skinningMorphSelectionDescriptorAllocateInfo,
      &mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp morph skinning selection descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute transform. global data */
    VkDescriptorSetAllocateInfo computeTransformDescriptorAllocateInfo{};
    computeTransformDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeTransformDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    computeTransformDescriptorAllocateInfo.descriptorSetCount = 1;
    computeTransformDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpComputeTransformDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &computeTransformDescriptorAllocateInfo,
      &mRenderData.rdAssimpComputeTransformDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Transform Compute descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* compute transform fpr bounding spheres. global data */
    VkDescriptorSetAllocateInfo computeTransformDescriptorAllocateInfo{};
    computeTransformDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeTransformDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    computeTransformDescriptorAllocateInfo.descriptorSetCount = 1;
    computeTransformDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpComputeTransformDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &computeTransformDescriptorAllocateInfo,
      &mRenderData.rdAssimpComputeSphereTransformDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Bounding Sphere Transform Compute descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* matrix multiplication, global data */
    VkDescriptorSetAllocateInfo computeMatrixMultDescriptorAllocateInfo{};
    computeMatrixMultDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeMatrixMultDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    computeMatrixMultDescriptorAllocateInfo.descriptorSetCount = 1;
    computeMatrixMultDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpComputeMatrixMultDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &computeMatrixMultDescriptorAllocateInfo,
      &mRenderData.rdAssimpComputeMatrixMultDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Matrix Mult Compute descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* matrix multiplication bounding spheres, global data */
    VkDescriptorSetAllocateInfo computeMatrixMultDescriptorAllocateInfo{};
    computeMatrixMultDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeMatrixMultDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    computeMatrixMultDescriptorAllocateInfo.descriptorSetCount = 1;
    computeMatrixMultDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpComputeMatrixMultDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &computeMatrixMultDescriptorAllocateInfo,
      &mRenderData.rdAssimpComputeSphereMatrixMultDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Bounding Sphere Matrix Mult Compute descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* bounding spheres, global data */
    VkDescriptorSetAllocateInfo computeBoundingSpheresDescriptorAllocateInfo{};
    computeBoundingSpheresDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    computeBoundingSpheresDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    computeBoundingSpheresDescriptorAllocateInfo.descriptorSetCount = 1;
    computeBoundingSpheresDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpComputeBoundingSpheresDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &computeBoundingSpheresDescriptorAllocateInfo,
      &mRenderData.rdAssimpComputeBoundingSpheresDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp Bounding Sphere Compute descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* line-drawing */
    VkDescriptorSetAllocateInfo lineAllocateInfo{};
    lineAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    lineAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    lineAllocateInfo.descriptorSetCount = 1;
    lineAllocateInfo.pSetLayouts = &mRenderData.rdLineDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &lineAllocateInfo,
      &mRenderData.rdLineDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp line-drawing descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  {
    /* sphere-drawing */
    VkDescriptorSetAllocateInfo sphereAllocateInfo{};
    sphereAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    sphereAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
    sphereAllocateInfo.descriptorSetCount = 1;
    sphereAllocateInfo.pSetLayouts = &mRenderData.rdSphereDescriptorLayout;

    VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &sphereAllocateInfo,
      &mRenderData.rdSphereDescriptorSet);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not allocate Assimp bounding sphere-drawing descriptor set (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  updateDescriptorSets();

  return true;
}

void VkRenderer::updateDescriptorSets() {
  /* we must update the descriptor sets whenever the buffer size has changed */
  {
    /* non-animated shader */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo worldPosInfo{};
    worldPosInfo.buffer = mShaderModelRootMatrixBuffer.buffer;
    worldPosInfo.offset = 0;
    worldPosInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo selectionInfo{};
    selectionInfo.buffer = mSelectedInstanceBuffer.buffer;
    selectionInfo.offset = 0;
    selectionInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    VkWriteDescriptorSet posWriteDescriptorSet{};
    posWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    posWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    posWriteDescriptorSet.dstSet = mRenderData.rdAssimpDescriptorSet;
    posWriteDescriptorSet.dstBinding = 1;
    posWriteDescriptorSet.descriptorCount = 1;
    posWriteDescriptorSet.pBufferInfo = &worldPosInfo;

    VkWriteDescriptorSet selectionWriteDescriptorSet{};
    selectionWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    selectionWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    selectionWriteDescriptorSet.dstSet = mRenderData.rdAssimpDescriptorSet;
    selectionWriteDescriptorSet.dstBinding = 2;
    selectionWriteDescriptorSet.descriptorCount = 1;
    selectionWriteDescriptorSet.pBufferInfo = &selectionInfo;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
       { matrixWriteDescriptorSet, posWriteDescriptorSet, selectionWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(writeDescriptorSets.size()),
       writeDescriptorSets.data(), 0, nullptr);
  }

  {
    /* animated shader */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneMatrixInfo{};
    boneMatrixInfo.buffer = mShaderBoneMatrixBuffer.buffer;
    boneMatrixInfo.offset = 0;
    boneMatrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo worldPosInfo{};
    worldPosInfo.buffer = mShaderModelRootMatrixBuffer.buffer;
    worldPosInfo.offset = 0;
    worldPosInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo selectionInfo{};
    selectionInfo.buffer = mSelectedInstanceBuffer.buffer;
    selectionInfo.offset = 0;
    selectionInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
    boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningDescriptorSet;
    boneMatrixWriteDescriptorSet.dstBinding = 1;
    boneMatrixWriteDescriptorSet.descriptorCount = 1;
    boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

    VkWriteDescriptorSet posWriteDescriptorSet{};
    posWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    posWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    posWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningDescriptorSet;
    posWriteDescriptorSet.dstBinding = 2;
    posWriteDescriptorSet.descriptorCount = 1;
    posWriteDescriptorSet.pBufferInfo = &worldPosInfo;

    VkWriteDescriptorSet selectionWriteDescriptorSet{};
    selectionWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    selectionWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    selectionWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningDescriptorSet;
    selectionWriteDescriptorSet.dstBinding = 3;
    selectionWriteDescriptorSet.descriptorCount = 1;
    selectionWriteDescriptorSet.pBufferInfo = &selectionInfo;

    std::vector<VkWriteDescriptorSet> skinningWriteDescriptorSets =
      { matrixWriteDescriptorSet, boneMatrixWriteDescriptorSet, posWriteDescriptorSet, selectionWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(skinningWriteDescriptorSets.size()),
       skinningWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* selection shader, non-animated  */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo worldPosInfo{};
    worldPosInfo.buffer = mShaderModelRootMatrixBuffer.buffer;
    worldPosInfo.offset = 0;
    worldPosInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo selectionInfo{};
    selectionInfo.buffer = mSelectedInstanceBuffer.buffer;
    selectionInfo.offset = 0;
    selectionInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSelectionDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    VkWriteDescriptorSet posWriteDescriptorSet{};
    posWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    posWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    posWriteDescriptorSet.dstSet = mRenderData.rdAssimpSelectionDescriptorSet;
    posWriteDescriptorSet.dstBinding = 1;
    posWriteDescriptorSet.descriptorCount = 1;
    posWriteDescriptorSet.pBufferInfo = &worldPosInfo;

    VkWriteDescriptorSet selectionWriteDescriptorSet{};
    selectionWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    selectionWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    selectionWriteDescriptorSet.dstSet = mRenderData.rdAssimpSelectionDescriptorSet;
    selectionWriteDescriptorSet.dstBinding = 2;
    selectionWriteDescriptorSet.descriptorCount = 1;
    selectionWriteDescriptorSet.pBufferInfo = &selectionInfo;

    std::vector<VkWriteDescriptorSet> selectionWriteDescriptorSets =
      { matrixWriteDescriptorSet, posWriteDescriptorSet, selectionWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(selectionWriteDescriptorSets.size()),
       selectionWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* selection shader, animated  */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneMatrixInfo{};
    boneMatrixInfo.buffer = mShaderBoneMatrixBuffer.buffer;
    boneMatrixInfo.offset = 0;
    boneMatrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo worldPosInfo{};
    worldPosInfo.buffer = mShaderModelRootMatrixBuffer.buffer;
    worldPosInfo.offset = 0;
    worldPosInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo selectionInfo{};
    selectionInfo.buffer = mSelectedInstanceBuffer.buffer;
    selectionInfo.offset = 0;
    selectionInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningSelectionDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
    boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningSelectionDescriptorSet;
    boneMatrixWriteDescriptorSet.dstBinding = 1;
    boneMatrixWriteDescriptorSet.descriptorCount = 1;
    boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

    VkWriteDescriptorSet posWriteDescriptorSet{};
    posWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    posWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    posWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningSelectionDescriptorSet;
    posWriteDescriptorSet.dstBinding = 2;
    posWriteDescriptorSet.descriptorCount = 1;
    posWriteDescriptorSet.pBufferInfo = &worldPosInfo;

    VkWriteDescriptorSet selectionWriteDescriptorSet{};
    selectionWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    selectionWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    selectionWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningSelectionDescriptorSet;
    selectionWriteDescriptorSet.dstBinding = 3;
    selectionWriteDescriptorSet.descriptorCount = 1;
    selectionWriteDescriptorSet.pBufferInfo = &selectionInfo;

    std::vector<VkWriteDescriptorSet> skinningSelectionWriteDescriptorSets =
      { matrixWriteDescriptorSet, boneMatrixWriteDescriptorSet,
        posWriteDescriptorSet, selectionWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(skinningSelectionWriteDescriptorSets.size()),
       skinningSelectionWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* animated plus morph shader */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneMatrixInfo{};
    boneMatrixInfo.buffer = mShaderBoneMatrixBuffer.buffer;
    boneMatrixInfo.offset = 0;
    boneMatrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo worldPosInfo{};
    worldPosInfo.buffer = mShaderModelRootMatrixBuffer.buffer;
    worldPosInfo.offset = 0;
    worldPosInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo selectionInfo{};
    selectionInfo.buffer = mSelectedInstanceBuffer.buffer;
    selectionInfo.offset = 0;
    selectionInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo faceAnimInfo{};
    faceAnimInfo.buffer = mFaceAnimPerInstanceDataBuffer.buffer;
    faceAnimInfo.offset = 0;
    faceAnimInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
    boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphDescriptorSet;
    boneMatrixWriteDescriptorSet.dstBinding = 1;
    boneMatrixWriteDescriptorSet.descriptorCount = 1;
    boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

    VkWriteDescriptorSet posWriteDescriptorSet{};
    posWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    posWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    posWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphDescriptorSet;
    posWriteDescriptorSet.dstBinding = 2;
    posWriteDescriptorSet.descriptorCount = 1;
    posWriteDescriptorSet.pBufferInfo = &worldPosInfo;

    VkWriteDescriptorSet selectionWriteDescriptorSet{};
    selectionWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    selectionWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    selectionWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphDescriptorSet;
    selectionWriteDescriptorSet.dstBinding = 3;
    selectionWriteDescriptorSet.descriptorCount = 1;
    selectionWriteDescriptorSet.pBufferInfo = &selectionInfo;

    VkWriteDescriptorSet faceAnimWriteDescriptorSet{};
    faceAnimWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    faceAnimWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    faceAnimWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphDescriptorSet;
    faceAnimWriteDescriptorSet.dstBinding = 4;
    faceAnimWriteDescriptorSet.descriptorCount = 1;
    faceAnimWriteDescriptorSet.pBufferInfo = &faceAnimInfo;

    std::vector<VkWriteDescriptorSet> skinningWriteDescriptorSets =
      { matrixWriteDescriptorSet, boneMatrixWriteDescriptorSet,
        posWriteDescriptorSet, selectionWriteDescriptorSet, faceAnimWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(skinningWriteDescriptorSets.size()),
       skinningWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* selection shader, animated plus morph */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneMatrixInfo{};
    boneMatrixInfo.buffer = mShaderBoneMatrixBuffer.buffer;
    boneMatrixInfo.offset = 0;
    boneMatrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo worldPosInfo{};
    worldPosInfo.buffer = mShaderModelRootMatrixBuffer.buffer;
    worldPosInfo.offset = 0;
    worldPosInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo selectionInfo{};
    selectionInfo.buffer = mSelectedInstanceBuffer.buffer;
    selectionInfo.offset = 0;
    selectionInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo faceAnimInfo{};
    faceAnimInfo.buffer = mFaceAnimPerInstanceDataBuffer.buffer;
    faceAnimInfo.offset = 0;
    faceAnimInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
    boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet;
    boneMatrixWriteDescriptorSet.dstBinding = 1;
    boneMatrixWriteDescriptorSet.descriptorCount = 1;
    boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

    VkWriteDescriptorSet posWriteDescriptorSet{};
    posWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    posWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    posWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet;
    posWriteDescriptorSet.dstBinding = 2;
    posWriteDescriptorSet.descriptorCount = 1;
    posWriteDescriptorSet.pBufferInfo = &worldPosInfo;

    VkWriteDescriptorSet selectionWriteDescriptorSet{};
    selectionWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    selectionWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    selectionWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet;
    selectionWriteDescriptorSet.dstBinding = 3;
    selectionWriteDescriptorSet.descriptorCount = 1;
    selectionWriteDescriptorSet.pBufferInfo = &selectionInfo;

    VkWriteDescriptorSet faceAnimWriteDescriptorSet{};
    faceAnimWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    faceAnimWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    faceAnimWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet;
    faceAnimWriteDescriptorSet.dstBinding = 4;
    faceAnimWriteDescriptorSet.descriptorCount = 1;
    faceAnimWriteDescriptorSet.pBufferInfo = &faceAnimInfo;

    std::vector<VkWriteDescriptorSet> skinningSelectionWriteDescriptorSets =
    { matrixWriteDescriptorSet, boneMatrixWriteDescriptorSet,
      posWriteDescriptorSet, selectionWriteDescriptorSet, faceAnimWriteDescriptorSet };

      vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(skinningSelectionWriteDescriptorSets.size()),
        skinningSelectionWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* line-drawing shader */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdLineDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    { matrixWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(writeDescriptorSets.size()),
      writeDescriptorSets.data(), 0, nullptr);
  }

}

void VkRenderer::updateComputeDescriptorSets() {
  {
    /* transform compute shader */
    VkDescriptorBufferInfo transformInfo{};
    transformInfo.buffer = mPerInstanceAnimDataBuffer.buffer;
    transformInfo.offset = 0;
    transformInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo trsInfo{};
    trsInfo.buffer = mShaderTRSMatrixBuffer.buffer;
    trsInfo.offset = 0;
    trsInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet transformWriteDescriptorSet{};
    transformWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    transformWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    transformWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeTransformDescriptorSet;
    transformWriteDescriptorSet.dstBinding = 0;
    transformWriteDescriptorSet.descriptorCount = 1;
    transformWriteDescriptorSet.pBufferInfo = &transformInfo;

    VkWriteDescriptorSet trsWriteDescriptorSet{};
    trsWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    trsWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    trsWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeTransformDescriptorSet;
    trsWriteDescriptorSet.dstBinding = 1;
    trsWriteDescriptorSet.descriptorCount = 1;
    trsWriteDescriptorSet.pBufferInfo = &trsInfo;

    std::vector<VkWriteDescriptorSet> transformWriteDescriptorSets =
      { transformWriteDescriptorSet, trsWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(transformWriteDescriptorSets.size()),
       transformWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* matrix multiplication compute shader, global data */
    VkDescriptorBufferInfo trsInfo{};
    trsInfo.buffer = mShaderTRSMatrixBuffer.buffer;
    trsInfo.offset = 0;
    trsInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneMatrixInfo{};
    boneMatrixInfo.buffer = mShaderBoneMatrixBuffer.buffer;
    boneMatrixInfo.offset = 0;
    boneMatrixInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet trsWriteDescriptorSet{};
    trsWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    trsWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    trsWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeMatrixMultDescriptorSet;
    trsWriteDescriptorSet.dstBinding = 0;
    trsWriteDescriptorSet.descriptorCount = 1;
    trsWriteDescriptorSet.pBufferInfo = &trsInfo;

    VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
    boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeMatrixMultDescriptorSet;
    boneMatrixWriteDescriptorSet.dstBinding = 1;
    boneMatrixWriteDescriptorSet.descriptorCount = 1;
    boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

    std::vector<VkWriteDescriptorSet> matrixMultWriteDescriptorSets =
      { trsWriteDescriptorSet, boneMatrixWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(matrixMultWriteDescriptorSets.size()),
       matrixMultWriteDescriptorSets.data(), 0, nullptr);
  }
}

void VkRenderer::updateSphereComputeDescriptorSets() {
  {
    /* transform compute shader for bounding spheres */
    VkDescriptorBufferInfo transformInfo{};
    transformInfo.buffer = mSpherePerInstanceAnimDataBuffer.buffer;
    transformInfo.offset = 0;
    transformInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo trsInfo{};
    trsInfo.buffer = mSphereTRSMatrixBuffer.buffer;
    trsInfo.offset = 0;
    trsInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet transformWriteDescriptorSet{};
    transformWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    transformWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    transformWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeSphereTransformDescriptorSet;
    transformWriteDescriptorSet.dstBinding = 0;
    transformWriteDescriptorSet.descriptorCount = 1;
    transformWriteDescriptorSet.pBufferInfo = &transformInfo;

    VkWriteDescriptorSet trsWriteDescriptorSet{};
    trsWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    trsWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    trsWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeSphereTransformDescriptorSet;
    trsWriteDescriptorSet.dstBinding = 1;
    trsWriteDescriptorSet.descriptorCount = 1;
    trsWriteDescriptorSet.pBufferInfo = &trsInfo;

    std::vector<VkWriteDescriptorSet> transformWriteDescriptorSets =
      { transformWriteDescriptorSet, trsWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(transformWriteDescriptorSets.size()),
       transformWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* matrix multiplication compute shader, global data */
    VkDescriptorBufferInfo trsInfo{};
    trsInfo.buffer = mSphereTRSMatrixBuffer.buffer;
    trsInfo.offset = 0;
    trsInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boneMatrixInfo{};
    boneMatrixInfo.buffer = mSphereBoneMatrixBuffer.buffer;
    boneMatrixInfo.offset = 0;
    boneMatrixInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet trsWriteDescriptorSet{};
    trsWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    trsWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    trsWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeSphereMatrixMultDescriptorSet;
    trsWriteDescriptorSet.dstBinding = 0;
    trsWriteDescriptorSet.descriptorCount = 1;
    trsWriteDescriptorSet.pBufferInfo = &trsInfo;

    VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
    boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeSphereMatrixMultDescriptorSet;
    boneMatrixWriteDescriptorSet.dstBinding = 1;
    boneMatrixWriteDescriptorSet.descriptorCount = 1;
    boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

    std::vector<VkWriteDescriptorSet> matrixMultWriteDescriptorSets =
      { trsWriteDescriptorSet, boneMatrixWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(matrixMultWriteDescriptorSets.size()),
       matrixMultWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* bounding spheres compute shader, global data */
    VkDescriptorBufferInfo boneMatrixInfo{};
    boneMatrixInfo.buffer = mSphereBoneMatrixBuffer.buffer;
    boneMatrixInfo.offset = 0;
    boneMatrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo worldPosInfo{};
    worldPosInfo.buffer = mSphereModelRootMatrixBuffer.buffer;
    worldPosInfo.offset = 0;
    worldPosInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boundingSphereInfo{};
    boundingSphereInfo.buffer = mBoundingSphereBuffer.buffer;
    boundingSphereInfo.offset = 0;
    boundingSphereInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
    boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeBoundingSpheresDescriptorSet;
    boneMatrixWriteDescriptorSet.dstBinding = 0;
    boneMatrixWriteDescriptorSet.descriptorCount = 1;
    boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

    VkWriteDescriptorSet worldPosWriteDescriptorSet{};
    worldPosWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    worldPosWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    worldPosWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeBoundingSpheresDescriptorSet;
    worldPosWriteDescriptorSet.dstBinding = 1;
    worldPosWriteDescriptorSet.descriptorCount = 1;
    worldPosWriteDescriptorSet.pBufferInfo = &worldPosInfo;

    VkWriteDescriptorSet boundingSphereWriteDescriptorSet{};
    boundingSphereWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boundingSphereWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boundingSphereWriteDescriptorSet.dstSet = mRenderData.rdAssimpComputeBoundingSpheresDescriptorSet;
    boundingSphereWriteDescriptorSet.dstBinding = 2;
    boundingSphereWriteDescriptorSet.descriptorCount = 1;
    boundingSphereWriteDescriptorSet.pBufferInfo = &boundingSphereInfo;

    std::vector<VkWriteDescriptorSet> boundingSphereWriteDescriptorSets =
      { boneMatrixWriteDescriptorSet, worldPosWriteDescriptorSet, boundingSphereWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(boundingSphereWriteDescriptorSets.size()),
       boundingSphereWriteDescriptorSets.data(), 0, nullptr);
  }

  {
    /* sphere-drawing shader */
    VkDescriptorBufferInfo matrixInfo{};
    matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
    matrixInfo.offset = 0;
    matrixInfo.range = VK_WHOLE_SIZE;

    VkDescriptorBufferInfo boundingSpheresInfo{};
    boundingSpheresInfo.buffer = mBoundingSphereBuffer.buffer;
    boundingSpheresInfo.offset = 0;
    boundingSpheresInfo.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet matrixWriteDescriptorSet{};
    matrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    matrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    matrixWriteDescriptorSet.dstSet = mRenderData.rdSphereDescriptorSet;
    matrixWriteDescriptorSet.dstBinding = 0;
    matrixWriteDescriptorSet.descriptorCount = 1;
    matrixWriteDescriptorSet.pBufferInfo = &matrixInfo;

    VkWriteDescriptorSet boundingSpheresWriteDescriptorSet{};
    boundingSpheresWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    boundingSpheresWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    boundingSpheresWriteDescriptorSet.dstSet = mRenderData.rdSphereDescriptorSet;
    boundingSpheresWriteDescriptorSet.dstBinding = 1;
    boundingSpheresWriteDescriptorSet.descriptorCount = 1;
    boundingSpheresWriteDescriptorSet.pBufferInfo = &boundingSpheresInfo;

    std::vector<VkWriteDescriptorSet> writeDescriptorSets =
    { matrixWriteDescriptorSet, boundingSpheresWriteDescriptorSet };

    vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(writeDescriptorSets.size()),
      writeDescriptorSets.data(), 0, nullptr);
  }
}

bool VkRenderer::createDepthBuffer() {
  VkExtent3D depthImageExtent = {
        mRenderData.rdVkbSwapchain.extent.width,
        mRenderData.rdVkbSwapchain.extent.height,
        1
  };

  mRenderData.rdDepthFormat = VK_FORMAT_D32_SFLOAT;

  VkImageCreateInfo depthImageInfo{};
  depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
  depthImageInfo.format = mRenderData.rdDepthFormat;
  depthImageInfo.extent = depthImageExtent;
  depthImageInfo.mipLevels = 1;
  depthImageInfo.arrayLayers = 1;
  depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VmaAllocationCreateInfo depthAllocInfo{};
  depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  depthAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkResult result = vmaCreateImage(mRenderData.rdAllocator, &depthImageInfo, &depthAllocInfo, &mRenderData.rdDepthImage, &mRenderData.rdDepthImageAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate depth buffer memory (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkImageViewCreateInfo depthImageViewinfo{};
  depthImageViewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  depthImageViewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthImageViewinfo.image = mRenderData.rdDepthImage;
  depthImageViewinfo.format = mRenderData.rdDepthFormat;
  depthImageViewinfo.subresourceRange.baseMipLevel = 0;
  depthImageViewinfo.subresourceRange.levelCount = 1;
  depthImageViewinfo.subresourceRange.baseArrayLayer = 0;
  depthImageViewinfo.subresourceRange.layerCount = 1;
  depthImageViewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

  result = vkCreateImageView(mRenderData.rdVkbDevice.device, &depthImageViewinfo, nullptr, &mRenderData.rdDepthImageView);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create depth buffer image view (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  return true;
}

bool VkRenderer::createSelectionImage() {
  VkExtent3D selectionImageExtent = {
        mRenderData.rdVkbSwapchain.extent.width,
        mRenderData.rdVkbSwapchain.extent.height,
        1
  };

  mRenderData.rdSelectionFormat = VK_FORMAT_R32_SFLOAT;

  VkImageCreateInfo selecImageInfo{};
  selecImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  selecImageInfo.imageType = VK_IMAGE_TYPE_2D;
  selecImageInfo.format = mRenderData.rdSelectionFormat;
  selecImageInfo.extent = selectionImageExtent;
  selecImageInfo.mipLevels = 1;
  selecImageInfo.arrayLayers = 1;
  selecImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  selecImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  selecImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo selectionAllocInfo{};
  selectionAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  selectionAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkResult result = vmaCreateImage(mRenderData.rdAllocator, &selecImageInfo, &selectionAllocInfo,
    &mRenderData.rdSelectionImage, &mRenderData.rdSelectionImageAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate selection buffer memory (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkImageViewCreateInfo selectionImageViewinfo{};
  selectionImageViewinfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  selectionImageViewinfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  selectionImageViewinfo.image = mRenderData.rdSelectionImage;
  selectionImageViewinfo.format = mRenderData.rdSelectionFormat;
  selectionImageViewinfo.subresourceRange.baseMipLevel = 0;
  selectionImageViewinfo.subresourceRange.levelCount = 1;
  selectionImageViewinfo.subresourceRange.baseArrayLayer = 0;
  selectionImageViewinfo.subresourceRange.layerCount = 1;
  selectionImageViewinfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

  result = vkCreateImageView(mRenderData.rdVkbDevice.device, &selectionImageViewinfo,
    nullptr, &mRenderData.rdSelectionImageView);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create selection buffer image view (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  return true;
}

bool VkRenderer::createSwapchain() {
  vkb::SwapchainBuilder swapChainBuild{mRenderData.rdVkbDevice};
  VkSurfaceFormatKHR surfaceFormat;

  /* set surface to sRGB */
  surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  //surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
  surfaceFormat.format = VK_FORMAT_B8G8R8A8_SRGB;

  /* VK_PRESENT_MODE_FIFO_KHR enables vsync */
  auto  swapChainBuildRet = swapChainBuild
    .set_old_swapchain(mRenderData.rdVkbSwapchain)
    .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
    .set_desired_format(surfaceFormat)
    .build();

  if (!swapChainBuildRet) {
    Logger::log(1, "%s error: could not init swapchain\n", __FUNCTION__);
    return false;
  }

  vkb::destroy_swapchain(mRenderData.rdVkbSwapchain);
  mRenderData.rdVkbSwapchain = swapChainBuildRet.value();

  return true;
}

bool VkRenderer::recreateSwapchain() {
  /* handle minimize */
  glfwGetFramebufferSize(mRenderData.rdWindow, &mRenderData.rdWidth, &mRenderData.rdHeight);
  while (mRenderData.rdWidth == 0 || mRenderData.rdHeight == 0) {
    glfwGetFramebufferSize(mRenderData.rdWindow, &mRenderData.rdWidth, &mRenderData.rdHeight);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(mRenderData.rdVkbDevice.device);

  /* cleanup */
  Framebuffer::cleanup(mRenderData);
  SelectionFramebuffer::cleanup(mRenderData);

  vkDestroyImageView(mRenderData.rdVkbDevice.device, mRenderData.rdSelectionImageView, nullptr);
  vmaDestroyImage(mRenderData.rdAllocator, mRenderData.rdSelectionImage, mRenderData.rdSelectionImageAlloc);

  vkDestroyImageView(mRenderData.rdVkbDevice.device, mRenderData.rdDepthImageView, nullptr);
  vmaDestroyImage(mRenderData.rdAllocator, mRenderData.rdDepthImage, mRenderData.rdDepthImageAlloc);

  mRenderData.rdVkbSwapchain.destroy_image_views(mRenderData.rdSwapchainImageViews);

  /* and recreate */
  if (!createSwapchain()) {
    Logger::log(1, "%s error: could not recreate swapchain\n", __FUNCTION__);
    return false;
  }

  if (!createDepthBuffer()) {
    Logger::log(1, "%s error: could not recreate depth buffer\n", __FUNCTION__);
    return false;
  }

  if (!createSelectionImage()) {
    Logger::log(1, "%s error: could not recreate selection buffer\n", __FUNCTION__);
    return false;
  }

  if (!createFramebuffer()) {
    Logger::log(1, "%s error: could not recreate framebuffers\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createVertexBuffers() {
  if (!VertexBuffer::init(mRenderData, mLineVertexBuffer, 1024)) {
    Logger::log(1, "%s error: could not create line vertex buffer\n", __FUNCTION__);
    return false;
  }

  if (!VertexBuffer::init(mRenderData, mSphereVertexBuffer, 1024)) {
    Logger::log(1, "%s error: could not create sphere vertex buffer\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createMatrixUBO() {
  if (!UniformBuffer::init(mRenderData, mPerspectiveViewMatrixUBO)) {
    Logger::log(1, "%s error: could not create matrix uniform buffers\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createSSBOs() {
  if (!ShaderStorageBuffer::init(mRenderData, mShaderTRSMatrixBuffer)) {
    Logger::log(1, "%s error: could not create TRS matrices SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mShaderModelRootMatrixBuffer)) {
    Logger::log(1, "%s error: could not create nodel root position SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mPerInstanceAnimDataBuffer)) {
    Logger::log(1, "%s error: could not create node transform SSBO\n", __FUNCTION__);
    return false;
  }

  /* we must read back data */
  if (!ShaderStorageBuffer::init(mRenderData, mShaderBoneMatrixBuffer)) {
    Logger::log(1, "%s error: could not create bone matrix SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mSelectedInstanceBuffer)) {
    Logger::log(1, "%s error: could not create selection SSBO\n", __FUNCTION__);
    return false;
  }

  /* we must read back data */
  if (!ShaderStorageBuffer::init(mRenderData, mBoundingSphereBuffer)) {
    Logger::log(1, "%s error: could not create bounding sphere SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mSphereModelRootMatrixBuffer)) {
    Logger::log(1, "%s error: could not create nodel root position SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mSpherePerInstanceAnimDataBuffer)) {
    Logger::log(1, "%s error: could not create node transform SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mSphereTRSMatrixBuffer)) {
    Logger::log(1, "%s error: could not create TRS matrices SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mSphereBoneMatrixBuffer)) {
    Logger::log(1, "%s error: could not create bone matrix SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mFaceAnimPerInstanceDataBuffer)) {
    Logger::log(1, "%s error: could not create face anim SSBO\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createRenderPass() {
  if (!Renderpass::init(mRenderData, mRenderData. rdRenderpass)) {
    Logger::log(1, "%s error: could not init renderpass\n", __FUNCTION__);
    return false;
  }

  if (!SecondaryRenderpass::init(mRenderData, mRenderData.rdImGuiRenderpass)) {
    Logger::log(1, "%s error: could not init ImGui renderpass\n", __FUNCTION__);
    return false;
  }

  if (!SecondaryRenderpass::init(mRenderData, mRenderData.rdLineRenderpass)) {
    Logger::log(1, "%s error: could not init line drawing renderpass\n", __FUNCTION__);
    return false;
  }

  if (!SelectionRenderpass::init(mRenderData)) {
    Logger::log(1, "%s error: could not init selection renderpass\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createPipelineLayouts() {
  /* non-animated model */
  std::vector<VkDescriptorSetLayout> layouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpDescriptorLayout };

  std::vector<VkPushConstantRange> pushConstants = { { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkPushConstants) } };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpPipelineLayout, layouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* animated model, needs push constant */
  std::vector<VkDescriptorSetLayout> skinningLayouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpSkinningDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpSkinningPipelineLayout, skinningLayouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp Skinning pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* selection, non-animated */
  std::vector<VkDescriptorSetLayout> selectionLayouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpSelectionDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpSelectionPipelineLayout, selectionLayouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp selection pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* selection, animated */
  std::vector<VkDescriptorSetLayout> skinningSelectionLayouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpSkinningSelectionDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpSkinningSelectionPipelineLayout, skinningSelectionLayouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp skinning selection pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* animated model plus morph */
  std::vector<VkDescriptorSetLayout> skinningMorphLayouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpSkinningMorphDescriptorLayout,
    mRenderData.rdAssimpSkinningMorphPerModelDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpSkinningMorphPipelineLayout, skinningMorphLayouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp morph skinning pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* selection, animated, morphs */
  std::vector<VkDescriptorSetLayout> skinningMorphSelectionLayouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpSkinningMorphSelectionDescriptorLayout,
    mRenderData.rdAssimpSkinningMorphPerModelDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpSkinningMorphSelectionPipelineLayout, skinningMorphSelectionLayouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp morph skinning selection pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* transform compute */
  std::vector<VkPushConstantRange> computePushConstants = { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(VkComputePushConstants) } };

  std::vector<VkDescriptorSetLayout> transformLayouts = {
    mRenderData.rdAssimpComputeTransformDescriptorLayout,
    mRenderData.rdAssimpComputeTransformPerModelDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpComputeTransformaPipelineLayout, transformLayouts, computePushConstants)) {
    Logger::log(1, "%s error: could not init Assimp transform compute pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* matrix mult compute */
  std::vector<VkDescriptorSetLayout> matrixMultLayouts = {
    mRenderData.rdAssimpComputeMatrixMultDescriptorLayout,
    mRenderData.rdAssimpComputeMatrixMultPerModelDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpComputeMatrixMultPipelineLayout, matrixMultLayouts, computePushConstants)) {
    Logger::log(1, "%s error: could not init Assimp matrix multiplication compute pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* bounding spheres compute */
  std::vector<VkDescriptorSetLayout> boundingSpheresLayouts = {
    mRenderData.rdAssimpComputeBoundingSpheresDescriptorLayout,
    mRenderData.rdAssimpComputeBoundingSpheresPerModelDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpComputeBoundingSpheresPipelineLayout, boundingSpheresLayouts, computePushConstants)) {
    Logger::log(1, "%s error: could not init Assimp bounding spheres compute pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* line drawing */
  std::vector<VkDescriptorSetLayout> lineLayouts = {
    mRenderData.rdLineDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdLinePipelineLayout, lineLayouts)) {
    Logger::log(1, "%s error: could not init Assimp line drawing pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* sphere drawing */
  std::vector<VkDescriptorSetLayout> sphereLayouts = {
    mRenderData.rdSphereDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdSpherePipelineLayout, sphereLayouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp sphere drawing pipeline layout\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createPipelines() {
  std::string vertexShaderFile = "shader/assimp.vert.spv";
  std::string fragmentShaderFile = "shader/assimp.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpPipelineLayout,
      mRenderData.rdAssimpPipeline, mRenderData.rdRenderpass, 1, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp shader pipeline\n", __FUNCTION__);
    return false;
  }

  vertexShaderFile = "shader/assimp_skinning.vert.spv";
  fragmentShaderFile = "shader/assimp_skinning.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpSkinningPipelineLayout,
      mRenderData.rdAssimpSkinningPipeline, mRenderData.rdRenderpass, 1, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Skinning shader pipeline\n", __FUNCTION__);
    return false;
  }

  vertexShaderFile = "shader/assimp_selection.vert.spv";
  fragmentShaderFile = "shader/assimp_selection.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpSelectionPipelineLayout,
      mRenderData.rdAssimpSelectionPipeline, mRenderData.rdSelectionRenderpass, 2,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Selection shader pipeline\n", __FUNCTION__);
    return false;
  }

  vertexShaderFile = "shader/assimp_skinning_selection.vert.spv";
  fragmentShaderFile = "shader/assimp_skinning_selection.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpSkinningSelectionPipelineLayout,
      mRenderData.rdAssimpSkinningSelectionPipeline, mRenderData.rdSelectionRenderpass, 2,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Skinning Selection shader pipeline\n", __FUNCTION__);
    return false;
  }

  vertexShaderFile = "shader/assimp_skinning_morph.vert.spv";
  fragmentShaderFile = "shader/assimp_skinning_morph.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpSkinningMorphPipelineLayout,
      mRenderData.rdAssimpSkinningMorphPipeline, mRenderData.rdRenderpass, 1, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Morph Anim Skinning shader pipeline\n", __FUNCTION__);
    return false;
  }

  vertexShaderFile = "shader/assimp_skinning_morph_selection.vert.spv";
  fragmentShaderFile = "shader/assimp_skinning_morph_selection.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpSkinningMorphSelectionPipelineLayout,
      mRenderData.rdAssimpSkinningMorphSelectionPipeline, mRenderData.rdSelectionRenderpass, 2,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Morph Anim Skinning Selection shader pipeline\n", __FUNCTION__);
    return false;
  }

  std::string computeShaderFile = "shader/assimp_instance_transform.comp.spv";
  if (!ComputePipeline::init(mRenderData, mRenderData.rdAssimpComputeTransformaPipelineLayout,
      mRenderData.rdAssimpComputeTransformPipeline, computeShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Transform compute shader pipeline\n", __FUNCTION__);
    return false;
  }

  computeShaderFile = "shader/assimp_instance_matrix_mult.comp.spv";
  if (!ComputePipeline::init(mRenderData, mRenderData.rdAssimpComputeMatrixMultPipelineLayout,
      mRenderData.rdAssimpComputeMatrixMultPipeline, computeShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Transform compute shader pipeline\n", __FUNCTION__);
    return false;
  }

  computeShaderFile = "shader/assimp_instance_bounding_spheres.comp.spv";
  if (!ComputePipeline::init(mRenderData, mRenderData.rdAssimpComputeBoundingSpheresPipelineLayout,
      mRenderData.rdAssimpComputeBoundingSpheresPipeline, computeShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Bounding Spheres compute shader pipeline\n", __FUNCTION__);
    return false;
  }

  computeShaderFile = "shader/assimp_instance_headmove_transform.comp.spv";
  if (!ComputePipeline::init(mRenderData, mRenderData.rdAssimpComputeTransformaPipelineLayout,
      mRenderData.rdAssimpComputeHeadMoveTransformPipeline, computeShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Head Movement Transform compute shader pipeline\n", __FUNCTION__);
    return false;
  }

  vertexShaderFile = "shader/line.vert.spv";
  fragmentShaderFile = "shader/line.frag.spv";
  if (!LinePipeline::init(mRenderData, mRenderData.rdLinePipelineLayout,
      mRenderData.rdLinePipeline, mRenderData.rdLineRenderpass,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp line drawing shader pipeline\n", __FUNCTION__);
    return false;
  }

  vertexShaderFile = "shader/sphere_instance.vert.spv";
  fragmentShaderFile = "shader/sphere_instance.frag.spv";
  if (!LinePipeline::init(mRenderData, mRenderData.rdSpherePipelineLayout,
      mRenderData.rdSpherePipeline, mRenderData.rdLineRenderpass,
      vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp line drawing shader pipeline\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createFramebuffer() {
  if (!Framebuffer::init(mRenderData)) {
    Logger::log(1, "%s error: could not init framebuffer\n", __FUNCTION__);
    return false;
  }
  if (!SelectionFramebuffer::init(mRenderData)) {
    Logger::log(1, "%s error: could not init selectonframebuffer\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createCommandPools() {
  if (!CommandPool::init(mRenderData, vkb::QueueType::graphics, mRenderData.rdCommandPool)) {
    Logger::log(1, "%s error: could not create graphics command pool\n", __FUNCTION__);
    return false;
  }

  /* use graphics queue if we have a shared queue  */
  vkb::QueueType computeQueue = mHasDedicatedComputeQueue ? vkb::QueueType::compute : vkb::QueueType::graphics;
  if (!CommandPool::init(mRenderData, computeQueue, mRenderData.rdComputeCommandPool)) {
    Logger::log(1, "%s error: could not create compute command pool\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createCommandBuffers() {
  if (!CommandBuffer::init(mRenderData,mRenderData.rdCommandPool, mRenderData.rdCommandBuffer)) {
    Logger::log(1, "%s error: could not create command buffers\n", __FUNCTION__);
    return false;
  }

  if (!CommandBuffer::init(mRenderData,mRenderData.rdCommandPool, mRenderData.rdImGuiCommandBuffer)) {
    Logger::log(1, "%s error: could not create ImGui command buffers\n", __FUNCTION__);
    return false;
  }

  if (!CommandBuffer::init(mRenderData,mRenderData.rdCommandPool, mRenderData.rdLineCommandBuffer)) {
    Logger::log(1, "%s error: could not create line drawing command buffers\n", __FUNCTION__);
    return false;
  }

  if (!CommandBuffer::init(mRenderData,mRenderData.rdComputeCommandPool, mRenderData.rdComputeCommandBuffer)) {
    Logger::log(1, "%s error: could not create compute command buffers\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createSyncObjects() {
  if (!SyncObjects::init(mRenderData)) {
    Logger::log(1, "%s error: could not create sync objects\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::initVma() {
  VmaAllocatorCreateInfo allocatorInfo{};
  allocatorInfo.physicalDevice = mRenderData.rdVkbPhysicalDevice.physical_device;
  allocatorInfo.device = mRenderData.rdVkbDevice.device;
  allocatorInfo.instance = mRenderData.rdVkbInstance.instance;

  VkResult result = vmaCreateAllocator(&allocatorInfo, &mRenderData.rdAllocator);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not init VMA (error %i)\n", __FUNCTION__, result);
    return false;
  }
  return true;
}

bool VkRenderer::initUserInterface() {
  if (!mUserInterface.init(mRenderData)) {
    Logger::log(1, "%s error: could not init ImGui\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::hasModel(std::string modelFileName) {
  auto modelIter =  std::find_if(mModelInstCamData.micModelList.begin(), mModelInstCamData.micModelList.end(),
    [modelFileName](const auto& model) {
      return model->getModelFileNamePath() == modelFileName || model->getModelFileName() == modelFileName;
    });
  return modelIter != mModelInstCamData.micModelList.end();
}

std::shared_ptr<AssimpModel> VkRenderer::getModel(std::string modelFileName) {
  auto modelIter =  std::find_if(mModelInstCamData.micModelList.begin(), mModelInstCamData.micModelList.end(),
    [modelFileName](const auto& model) {
      return model->getModelFileNamePath() == modelFileName || model->getModelFileName() == modelFileName;
    });
  if (modelIter != mModelInstCamData.micModelList.end()) {
    return *modelIter;
  }
  return nullptr;
}

bool VkRenderer::addModel(std::string modelFileName, bool addInitialInstance, bool withUndo) {
  if (hasModel(modelFileName)) {
    Logger::log(1, "%s warning: model '%s' already existed, skipping\n", __FUNCTION__, modelFileName.c_str());
    return false;
  }

  std::shared_ptr<AssimpModel> model = std::make_shared<AssimpModel>();
  if (!model->loadModel(mRenderData, modelFileName)) {
    Logger::log(1, "%s error: could not load model file '%s'\n", __FUNCTION__, modelFileName.c_str());
    return false;
  }

  mModelInstCamData.micModelList.emplace_back(model);

  int prevSelectedModelId = mModelInstCamData.micSelectedModel;
  int prevSelectedInstanceId = mModelInstCamData.micSelectedInstance;

  std::shared_ptr<AssimpInstance> firstInstance;
  if (addInitialInstance) {
    /* also add a new instance here to see the model, but skip undo recording the new instance */
    firstInstance = addInstance(model, false);
    if (!firstInstance) {
      Logger::log(1, "%s error: could not add initial instance for model '%s'\n", __FUNCTION__, modelFileName.c_str());
      return false;
    }

    /* center the first real model instance */
    if (mModelInstCamData.micAssimpInstances.size() == 2) {
      centerInstance(firstInstance);
    }
  }

  /* select new model and new instance */
  mModelInstCamData.micSelectedModel = mModelInstCamData.micModelList.size() - 1;
  mModelInstCamData.micSelectedInstance = mModelInstCamData.micAssimpInstances.size() - 1;

  if (withUndo) {
    mModelInstCamData.micSettingsContainer->applyLoadModel(model, mModelInstCamData.micSelectedModel, firstInstance,
      mModelInstCamData.micSelectedModel, prevSelectedModelId,
      mModelInstCamData.micSelectedInstance, prevSelectedInstanceId);
  }

  /* create AABBs for the model */
  if (!createAABBLookup(model)) {
    return false;
  }

  return true;
}

void VkRenderer::addExistingModel(std::shared_ptr<AssimpModel> model, int indexPos) {
  Logger::log(2, "%s: inserting model %s on pos %i\n", __FUNCTION__, model->getModelFileName().c_str(), indexPos);
  mModelInstCamData.micModelList.insert(mModelInstCamData.micModelList.begin() + indexPos, model);
}

void VkRenderer::deleteModel(std::string modelFileName, bool withUndo) {
  std::string shortModelFileName = std::filesystem::path(modelFileName).filename().generic_string();

  int prevSelectedModelId = mModelInstCamData.micSelectedModel;
  int prevSelectedInstanceId = mModelInstCamData.micSelectedInstance;

  mModelInstCamData.micAssimpInstances.erase(
    std::remove_if(
      mModelInstCamData.micAssimpInstances.begin(),
      mModelInstCamData.micAssimpInstances.end(),
      [shortModelFileName](std::shared_ptr<AssimpInstance> instance) { return instance->getModel()->getModelFileName() == shortModelFileName; }
    ), mModelInstCamData.micAssimpInstances.end()
  );

  std::vector<std::shared_ptr<AssimpInstance>> deletedInstances;
  std::shared_ptr<AssimpModel> model = getModel(modelFileName);

  auto modelIndex = std::find_if(mModelInstCamData.micModelList.begin(), mModelInstCamData.micModelList.end(),
    [modelFileName](std::shared_ptr<AssimpModel> model) { return model->getModelFileName() == modelFileName; }
  );

  size_t indexPos = mModelInstCamData.micModelList.size() - 1;
  if (modelIndex != mModelInstCamData.micModelList.end()) {
    indexPos = modelIndex - mModelInstCamData.micModelList.begin();
  }

  if (mModelInstCamData.micAssimpInstancesPerModel.count(shortModelFileName) > 0) {
    deletedInstances.swap(mModelInstCamData.micAssimpInstancesPerModel[shortModelFileName]);
  }

  /* save model in separate pending deletion list before purging from model list */
  if (model) {
    mModelInstCamData.micPendingDeleteAssimpModels.insert(model);
  }

  mModelInstCamData.micModelList.erase(
    std::remove_if(
      mModelInstCamData.micModelList.begin(),
      mModelInstCamData.micModelList.end(),
      [modelFileName](std::shared_ptr<AssimpModel> model) {
        return model->getModelFileName() == modelFileName; }
    )
  );

  /* decrement selected model index to point to model that is in list before the deleted one */
  if (mModelInstCamData.micSelectedModel > 1) {
    mModelInstCamData.micSelectedModel -= 1;
  }

  /* reset model instance to first instance */
  if (mModelInstCamData.micAssimpInstances.size() > 1) {
    mModelInstCamData.micSelectedInstance = 1;
  }

  /* if we have only the null instance left, disable selection */
  if (mModelInstCamData.micAssimpInstances.size() == 1) {
    mModelInstCamData.micSelectedInstance = 0;
    mRenderData.rdHighlightSelectedInstance = false;
  }

  if (withUndo) {
    mModelInstCamData.micSettingsContainer->applyDeleteModel(model, indexPos, deletedInstances,
      mModelInstCamData.micSelectedModel, prevSelectedModelId,
      mModelInstCamData.micSelectedInstance, prevSelectedInstanceId);
  }

  enumerateInstances();
  updateTriangleCount();
}

std::shared_ptr<AssimpInstance> VkRenderer::getInstanceById(int instanceId) {
  if (instanceId < mModelInstCamData.micAssimpInstances.size()) {
    return mModelInstCamData.micAssimpInstances.at(instanceId);
  } else {
    Logger::log(1, "%s error: instance id %i out of range, we only have %i instances\n", __FUNCTION__, instanceId,  mModelInstCamData.micAssimpInstances.size());
    return mModelInstCamData.micAssimpInstances.at(0);
  }
}

std::shared_ptr<AssimpInstance> VkRenderer::addInstance(std::shared_ptr<AssimpModel> model, bool withUndo) {
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
  mModelInstCamData.micAssimpInstances.emplace_back(newInstance);
  mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);

  int prevSelectedInstanceId = mModelInstCamData.micSelectedInstance;

  /* select new instance */
  mModelInstCamData.micSelectedInstance = mModelInstCamData.micAssimpInstances.size() - 1;
  if (withUndo) {
    mModelInstCamData.micSettingsContainer->applyNewInstance(newInstance, mModelInstCamData.micSelectedInstance, prevSelectedInstanceId);
  }

  enumerateInstances();
  updateTriangleCount();

  return newInstance;
}

void VkRenderer::addExistingInstance(std::shared_ptr<AssimpInstance> instance, int indexPos, int indexPerModelPos) {
  Logger::log(2, "%s: inserting instance on pos %i\n", __FUNCTION__, indexPos);
  mModelInstCamData.micAssimpInstances.insert(mModelInstCamData.micAssimpInstances.begin() + indexPos, instance);
  mModelInstCamData.micAssimpInstancesPerModel[instance->getModel()->getModelFileName()].insert(
    mModelInstCamData.micAssimpInstancesPerModel[instance->getModel()->getModelFileName()].begin() +
    indexPerModelPos, instance);

  enumerateInstances();
  updateTriangleCount();
}

void VkRenderer::addInstances(std::shared_ptr<AssimpModel> model, int numInstances) {
  size_t animClipNum = model->getAnimClips().size();
  std::vector<std::shared_ptr<AssimpInstance>> newInstances;
  for (int i = 0; i < numInstances; ++i) {
    int xPos = std::rand() % 250 - 125;
    int zPos = std::rand() % 250 - 125;
    int rotation = std::rand() % 360 - 180;
    int clipNr = std::rand() % animClipNum;
    float animSpeed = (std::rand() % 50 + 75) / 100.0f;

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model, glm::vec3(xPos, 0.0f, zPos), glm::vec3(0.0f, rotation, 0.0f));
    if (animClipNum > 0) {
      InstanceSettings instSettings = newInstance->getInstanceSettings();
      instSettings.isFirstAnimClipNr = clipNr;
      instSettings.isSecondAnimClipNr = clipNr;
      instSettings.isAnimSpeedFactor = animSpeed;
      instSettings.isAnimBlendFactor = 0.0f;
      newInstance->setInstanceSettings(instSettings);
    }
    newInstances.emplace_back(newInstance);
    mModelInstCamData.micAssimpInstances.emplace_back(newInstance);
    mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }

  int prevSelectedInstanceId = mModelInstCamData.micSelectedInstance;

  /* select new instance */
  mModelInstCamData.micSelectedInstance = mModelInstCamData.micAssimpInstances.size() - 1;
  mModelInstCamData.micSettingsContainer->applyNewMultiInstance(newInstances, mModelInstCamData.micSelectedInstance, prevSelectedInstanceId);

  enumerateInstances();
  updateTriangleCount();
}

void VkRenderer::deleteInstance(std::shared_ptr<AssimpInstance> instance, bool withUndo) {
  std::shared_ptr<AssimpModel> currentModel = instance->getModel();
  std::string currentModelName = currentModel->getModelFileName();

  mModelInstCamData.micAssimpInstances.erase(
    std::remove_if(
      mModelInstCamData.micAssimpInstances.begin(),
      mModelInstCamData.micAssimpInstances.end(),
      [instance](std::shared_ptr<AssimpInstance> inst) { return inst == instance; }),
      mModelInstCamData.micAssimpInstances.end());

  mModelInstCamData.micAssimpInstancesPerModel[currentModelName].erase(
    std::remove_if(
      mModelInstCamData.micAssimpInstancesPerModel[currentModelName].begin(),
      mModelInstCamData.micAssimpInstancesPerModel[currentModelName].end(),
      [instance](std::shared_ptr<AssimpInstance> inst) { return inst == instance; }),
      mModelInstCamData.micAssimpInstancesPerModel[currentModelName].end());

  int prevSelectedInstanceId = mModelInstCamData.micSelectedInstance;

  /* reset to last element if I was last */
  if (mModelInstCamData.micSelectedInstance > 1) {
    mModelInstCamData.micSelectedInstance -= 1;
  }

  if (withUndo) {
    mModelInstCamData.micSettingsContainer->applyDeleteInstance(instance, mModelInstCamData.micSelectedInstance, prevSelectedInstanceId);
  }

  enumerateInstances();
  updateTriangleCount();
}

void VkRenderer::cloneInstance(std::shared_ptr<AssimpInstance> instance) {
  std::shared_ptr<AssimpModel> currentModel = instance->getModel();
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(currentModel);
  InstanceSettings newInstanceSettings = instance->getInstanceSettings();

  /* slight offset to see new instance */
  newInstanceSettings.isWorldPosition += glm::vec3(1.0f, 0.0f, -1.0f);
  newInstance->setInstanceSettings(newInstanceSettings);

  mModelInstCamData.micAssimpInstances.emplace_back(newInstance);
  mModelInstCamData.micAssimpInstancesPerModel[currentModel->getModelFileName()].emplace_back(newInstance);

  int prevSelectedInstanceId = mModelInstCamData.micSelectedInstance;

  /* select new instance */
  mModelInstCamData.micSelectedInstance = mModelInstCamData.micAssimpInstances.size() - 1;
  mModelInstCamData.micSettingsContainer->applyNewInstance(newInstance, mModelInstCamData.micSelectedInstance, prevSelectedInstanceId);

  enumerateInstances();

  /* add behavior tree after new id was set */
  newInstanceSettings = newInstance->getInstanceSettings();
  if (!newInstanceSettings.isNodeTreeName.empty()) {
    addBehavior(newInstanceSettings.isInstanceIndexPosition, mModelInstCamData.micBehaviorData.at(newInstanceSettings.isNodeTreeName));
  }

  updateTriangleCount();
}

/* keep scaling and axis flipping */
void VkRenderer::cloneInstances(std::shared_ptr<AssimpInstance> instance, int numClones) {
  std::shared_ptr<AssimpModel> model = instance->getModel();
  std::vector<std::shared_ptr<AssimpInstance>> newInstances;
  for (int i = 0; i < numClones; ++i) {
    int xPos = std::rand() % 250 - 125;
    int zPos = std::rand() % 250 - 125;
    int rotation = std::rand() % 360 - 180;

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
    InstanceSettings instSettings = instance->getInstanceSettings();
    instSettings.isWorldPosition = glm::vec3(xPos, 0.0f, zPos);
    instSettings.isWorldRotation = glm::vec3(0.0f, rotation, 0.0f);

    newInstance->setInstanceSettings(instSettings);

    newInstances.emplace_back(newInstance);
    mModelInstCamData.micAssimpInstances.emplace_back(newInstance);
    mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }

  enumerateInstances();

  /* add behavior tree after new id was set */
  for (int i = 0; i < numClones; ++i) {
    InstanceSettings newInstanceSettings = newInstances.at(i)->getInstanceSettings();
    if (!newInstanceSettings.isNodeTreeName.empty()) {
      addBehavior(newInstanceSettings.isInstanceIndexPosition, mModelInstCamData.micBehaviorData.at(newInstanceSettings.isNodeTreeName));
    }
  }

  int prevSelectedInstanceId = mModelInstCamData.micSelectedInstance;

  /* select new instance */
  mModelInstCamData.micSelectedInstance = mModelInstCamData.micAssimpInstances.size() - 1;
  mModelInstCamData.micSettingsContainer->applyNewMultiInstance(newInstances, mModelInstCamData.micSelectedInstance, prevSelectedInstanceId);

  updateTriangleCount();
}

void VkRenderer::centerInstance(std::shared_ptr<AssimpInstance> instance) {
  InstanceSettings instSettings = instance->getInstanceSettings();
  mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera)->moveCameraTo(instSettings.isWorldPosition + glm::vec3(5.0f));
}

std::vector<glm::vec2> VkRenderer::get2DPositionOfAllInstances() {
  std::vector<glm::vec2> positions;

  /* skip null instance */
  for (int i = 1; i < mModelInstCamData.micAssimpInstances.size(); ++i) {
    glm::vec3 modelPos = mModelInstCamData.micAssimpInstances.at(i)->getWorldPosition();
    positions.emplace_back(modelPos.x, modelPos.z);
  }

  return positions;
}

void VkRenderer::editGraph(std::string graphName) {
  if (mModelInstCamData.micBehaviorData.count(graphName) > 0) {
    mGraphEditor->loadData(mModelInstCamData.micBehaviorData.at(graphName)->getBehaviorData());
  } else {
    Logger::log(1, "%s error: graph '%s' not found\n", __FUNCTION__, graphName.c_str());
  }
}

std::shared_ptr<SingleInstanceBehavior> VkRenderer::createEmptyGraph() {
  mGraphEditor->createEmptyGraph();
  return mGraphEditor->getData();
}

void VkRenderer::initQuadTree(int thresholdPerBox, int maxDepth) {
  mQuadtree = std::make_shared<QuadTree>(mWorldBoundaries, thresholdPerBox, maxDepth);

  /* quadtree needs to get bounding box of the instances */
  mQuadtree->mInstanceGetBoundingBox2DCallbackFunction = [this](int instanceId) {
    return mModelInstCamData.micAssimpInstances.at(instanceId)->getBoundingBox();
  };
}

std::shared_ptr<BoundingBox2D> VkRenderer::getWorldBoundaries() {
  return mWorldBoundaries;
}

void VkRenderer::addBehavior(int instanceId, std::shared_ptr<SingleInstanceBehavior> behavior) {
  if (mModelInstCamData.micAssimpInstances.size() < instanceId) {
    Logger::log(1, "%s error: number of instances is smaller than instance id %i\n", __FUNCTION__, instanceId);
    return;
  }

  mBehviorTimer.start();
  mBehavior->addInstance(instanceId, behavior);
  mRenderData.rdBehaviorTime += mBehviorTimer.stop();
  Logger::log(1, "%s: added behavior %s to instance %i\n", __FUNCTION__, behavior->getBehaviorData()->bdName.c_str(), instanceId);
}

void VkRenderer::delBehavior(int instanceId) {
  if (mModelInstCamData.micAssimpInstances.size() < instanceId) {
    Logger::log(1, "%s error: number of instances is smaller than instance id %i\n", __FUNCTION__, instanceId);
    return;
  }

  mBehviorTimer.start();
  mBehavior->removeInstance(instanceId);
  mRenderData.rdBehaviorTime += mBehviorTimer.stop();

  Logger::log(1, "%s: removed behavior from instance %i\n", __FUNCTION__, instanceId);
}

void VkRenderer::addModelBehavior(std::string modelName, std::shared_ptr<SingleInstanceBehavior> behavior) {
  std::shared_ptr<AssimpModel> model = getModel(modelName);
  if (!model) {
    Logger::log(1, "%s error: model %s not found\n", __FUNCTION__, modelName.c_str());
    return;
  }

  for (auto& instance : mModelInstCamData.micAssimpInstancesPerModel[modelName]) {
    InstanceSettings settings = instance->getInstanceSettings();
    mBehavior->addInstance(settings.isInstanceIndexPosition, behavior);
    settings.isNodeTreeName = behavior->getBehaviorData()->bdName;
    instance->setInstanceSettings(settings);
  }

  Logger::log(1, "%s: added behavior %s to all instances of model %s\n", __FUNCTION__, behavior->getBehaviorData()->bdName.c_str(), modelName.c_str());
}

void VkRenderer::delModelBehavior(std::string modelName) {
  std::shared_ptr<AssimpModel> model = getModel(modelName);
  if (!model) {
    Logger::log(1, "%s error: model %s not found\n", __FUNCTION__, modelName.c_str());
    return;
  }

  for (auto& instance : mModelInstCamData.micAssimpInstancesPerModel[modelName]) {
    InstanceSettings settings = instance->getInstanceSettings();
    mBehavior->removeInstance(settings.isInstanceIndexPosition);
    settings.isNodeTreeName.clear();
    instance->setInstanceSettings(settings);

    /* works here because we don't edit instances */
    instance->stopInstance();
  }

  Logger::log(1, "%s: removed behavior from all instances of model %s\n", __FUNCTION__, modelName.c_str());
}

void VkRenderer::updateInstanceSettings(int instanceId, graphNodeType nodeType,
    instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
  if (instanceId >= mModelInstCamData.micAssimpInstances.size()) {
    Logger::log(1, "%s error: number of instances is smaller than instance id %i\n", __FUNCTION__, instanceId);
    return;
  }
  std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(instanceId);
  InstanceSettings settings = instance->getInstanceSettings();
  moveDirection dir = settings.isMoveDirection;
  moveState state = settings.isMoveState;

  switch (nodeType) {
    case graphNodeType::instance:
      switch (updateType) {
        case instanceUpdateType::moveDirection:
          dir = std::get<moveDirection>(data);
          instance->updateInstanceState(state, dir);
          break;
        case instanceUpdateType::moveState:
          state = std::get<moveState>(data);
          instance->updateInstanceState(state, dir);
          break;
        case instanceUpdateType::speed:
          instance->setForwardSpeed(std::get<float>(data));
          break;
        case instanceUpdateType::rotation:
          /* true if relative rotation */
          if (extraSetting) {
            instance->rotateInstance(std::get<float>(data));
          } else {
            glm::vec3 currentRotation = instance->getRotation();
            instance->setRotation(glm::vec3(currentRotation.x, std::get<float>(data), currentRotation.z));
          }
          break;
        case instanceUpdateType::position:
          instance->setWorldPosition(std::get<glm::vec3>(data));
          break;
        default:
          /* do nothing */
          break;
      }
    case graphNodeType::action:
      if (updateType == instanceUpdateType::moveState) {
        state = std::get<moveState>(data);
        instance->setNextInstanceState(state);
      }
      break;
    case graphNodeType::faceAnim:
      switch (updateType) {
        case instanceUpdateType::faceAnimIndex:
          instance->setFaceAnim(std::get<faceAnimation>(data));
          break;
        case instanceUpdateType::faceAnimWeight:
          instance->setFaceAnimWeight(std::get<float>(data));
          break;
        default:
          /* do nothing */
          break;
      }
      break;
    case graphNodeType::headAmin:
      switch (updateType) {
        case instanceUpdateType::headAnim:
          instance->setHeadAnim(std::get<glm::vec2>(data));
          break;
        default:
          /* do nothing */
          break;
      }
      break;
    default:
      /* do nothing */
      break;
  }
}

void VkRenderer::addBehaviorEvent(int instanceId, nodeEvent event) {
  mBehavior->addEvent(instanceId, event);
}

void VkRenderer::postDelNodeTree(std::string nodeTreeName) {
  for (auto& instance : mModelInstCamData.micAssimpInstances) {
    InstanceSettings settings = instance->getInstanceSettings();
    if (settings.isNodeTreeName == nodeTreeName) {
      mBehavior->removeInstance(settings.isInstanceIndexPosition);
      settings.isNodeTreeName.clear();
    }
    instance->setInstanceSettings(settings);

    instance->stopInstance();
  }

  if (mGraphEditor->getCurrentEditedTreeName() == nodeTreeName) {
    mGraphEditor->closeEditor();
  }
}

void VkRenderer::updateTriangleCount() {
  mRenderData.rdTriangleCount = 0;
  for (const auto& instance : mModelInstCamData.micAssimpInstances) {
    mRenderData.rdTriangleCount += instance->getModel()->getTriangleCount();
  }
}

void VkRenderer::enumerateInstances() {
  for (size_t i = 0; i < mModelInstCamData.micAssimpInstances.size(); ++i) {
    InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(i)->getInstanceSettings();
    instSettings.isInstanceIndexPosition = i;
    mModelInstCamData.micAssimpInstances.at(i)->setInstanceSettings(instSettings);
  }

  for (const auto& modelType : mModelInstCamData.micAssimpInstancesPerModel) {
    for (size_t i = 0; i < modelType.second.size(); ++i) {
      InstanceSettings instSettings = modelType.second.at(i)->getInstanceSettings();
      instSettings.isInstancePerModelIndexPosition = i;
      modelType.second.at(i)->setInstanceSettings(instSettings);
    }
  }

  /* update also when number of instances has changed */
  mQuadtree->clear();
  /* skip null instance */
  for (size_t i = 1; i < mModelInstCamData.micAssimpInstances.size(); ++i) {
    mQuadtree->add(mModelInstCamData.micAssimpInstances.at(i)->getInstanceSettings().isInstanceIndexPosition);
  }

}

void VkRenderer::cloneCamera() {
  std::shared_ptr<Camera> currentCam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
  std::shared_ptr<Camera> newCam = std::make_shared<Camera>();

  CameraSettings settings = currentCam->getCameraSettings();
  settings.csCamName = generateUniqueCameraName(settings.csCamName);
  newCam->setCameraSettings(settings);

  mModelInstCamData.micCameras.emplace_back(newCam);
  mModelInstCamData.micSelectedCamera = mModelInstCamData.micCameras.size() - 1;
}

void VkRenderer::deleteCamera() {
  mModelInstCamData.micCameras.erase(mModelInstCamData.micCameras.begin() + mModelInstCamData.micSelectedCamera);
  mModelInstCamData.micSelectedCamera = mModelInstCamData.micCameras.size() - 1;
}

std::string VkRenderer::generateUniqueCameraName(std::string camBaseName) {
  std::string camName = camBaseName;
  while (checkCameraNameUsed(camName)) {
    char lastChar = camName.back();
    if (!std::isdigit(lastChar)) {
      camName.append("1");
    } else {
      std::string::size_type sz;
      std::string lastCharString(1, lastChar);
      int lastDigit = std::stoi(lastCharString, &sz);
      if (lastDigit != 9) {
        camName.pop_back();
        camName.append(std::to_string(lastDigit + 1));
      } else {
        camName.pop_back();
        camName.append("10");
      }
    }
  }
  return camName;
}

bool VkRenderer::checkCameraNameUsed(std::string cameraName) {
  for (const auto& cam : mModelInstCamData.micCameras) {
    if (cam->getCameraSettings().csCamName == cameraName) {
      return true;
    }
  }

  return false;
}

void VkRenderer::setSize(unsigned int width, unsigned int height) {
  /* handle minimize */
  if (width == 0 || height == 0) {
    return;
  }

  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  /* Vulkan detects changes and recreates swapchain */
  Logger::log(1, "%s: resized window to %ix%i\n", __FUNCTION__, width, height);
}

void VkRenderer::setConfigDirtyFlag(bool flag) {
  mConfigIsDirty = flag;
  if (mConfigIsDirty) {
    mWindowTitleDirtySign = "*";
  } else {
    mWindowTitleDirtySign = " ";
  }
  setModeInWindowTitle();
}

bool VkRenderer::getConfigDirtyFlag() {
  return mConfigIsDirty;
}

void VkRenderer::setModeInWindowTitle() {
  mModelInstCamData.micSetWindowTitleFunction(mOrigWindowTitle + " (" +
  mRenderData.mAppModeMap.at(mRenderData.rdApplicationMode) + " Mode)" +
  mWindowTitleDirtySign);
}

void VkRenderer::toggleFullscreen() {
  mRenderData.rdFullscreen = mRenderData.rdFullscreen ? false : true;

  static int xPos = 0;
  static int yPos = 0;
  static int width = mRenderData.rdWidth;
  static int height = mRenderData.rdHeight;
  if (mRenderData.rdFullscreen) {
    /* save position and resolution */
    glfwGetWindowPos(mRenderData.rdWindow, &xPos, &yPos);
    glfwGetWindowSize(mRenderData.rdWindow, &width, &height);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(mRenderData.rdWindow, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
  } else {
    glfwSetWindowMonitor(mRenderData.rdWindow, nullptr, xPos, yPos, width, height, 0);
  }
}

void VkRenderer::checkMouseEnable() {
  if (mMouseLock || mMouseMove || mRenderData.rdApplicationMode != appMode::edit) {
    glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    /* enable raw mode if possible */
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(mRenderData.rdWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
  } else {
    glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
}

void VkRenderer::handleKeyEvents(int key, int scancode, int action, int mods) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();

    /* hide from application if above ImGui window */
    if (io.WantCaptureKeyboard || io.WantTextInput) {
      return;
    }
  }

  /* toggle between edit and view mode by pressing F10 */
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_F10) == GLFW_PRESS) {
    int currentMode = static_cast<int>(mRenderData.rdApplicationMode);
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      mRenderData.rdApplicationMode = static_cast<appMode>((--currentMode + 2) % 2);
    } else {
      mRenderData.rdApplicationMode = static_cast<appMode>(++currentMode % 2);
    }
    setModeInWindowTitle();
  }

  /* toggle between full-screen and window mode by pressing F11 */
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_F11) == GLFW_PRESS) {
    toggleFullscreen();
  }

  if (mRenderData.rdApplicationMode == appMode::edit) {
    /* instance edit modes */
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_1) == GLFW_PRESS) {
      instanceEditMode oldMode = mRenderData.rdInstanceEditMode;
      mRenderData.rdInstanceEditMode = instanceEditMode::move;
      mModelInstCamData.micSettingsContainer->applyChangeEditMode(mRenderData.rdInstanceEditMode, oldMode);
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_2) == GLFW_PRESS) {
      instanceEditMode oldMode = mRenderData.rdInstanceEditMode;
      mRenderData.rdInstanceEditMode = instanceEditMode::rotate;
      mModelInstCamData.micSettingsContainer->applyChangeEditMode(mRenderData.rdInstanceEditMode, oldMode);
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_3) == GLFW_PRESS) {
      instanceEditMode oldMode = mRenderData.rdInstanceEditMode;
      mRenderData.rdInstanceEditMode = instanceEditMode::scale;
      mModelInstCamData.micSettingsContainer->applyChangeEditMode(mRenderData.rdInstanceEditMode, oldMode);
    }

    /* undo/redo only in edit mode */
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Z) == GLFW_PRESS &&
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
      undoLastOperation();
    }

    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Y) == GLFW_PRESS &&
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
      redoLastOperation();
    }

    /* new config/load/save keyboard shortcuts */
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_N) == GLFW_PRESS &&
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
      mRenderData.rdNewConfigRequest = true;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_L) == GLFW_PRESS &&
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
      mRenderData.rdLoadConfigRequest = true;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_S) == GLFW_PRESS &&
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
      glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
      mRenderData.rdSaveConfigRequest = true;
    }
  }

  /* exit via CTRL+Q, allow in edit and view mode */
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Q) == GLFW_PRESS &&
    (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
    glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)) {
    requestExitApplication();
  }

  /* toggle moving instance on Y axis when SHIFT is pressed */
  /* hack to react to both shift keys - remember which one was pressed */
  if (mMouseMove) {
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_LEFT_SHIFT;
      mMouseMoveVertical = true;
    }
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_RIGHT_SHIFT;
      mMouseMoveVertical = true;
    }
  }
  if  (glfwGetKey(mRenderData.rdWindow, mMouseMoveVerticalShiftKey) == GLFW_RELEASE) {
    mMouseMoveVerticalShiftKey = 0;
    mMouseMoveVertical = false;
  }

  /* switch cameras forward and backwards with square brackets, active in edit AND view mode */
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
    if (mModelInstCamData.micSelectedCamera > 0) {
      mModelInstCamData.micSelectedCamera--;
    }
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
    if (mModelInstCamData.micSelectedCamera < mModelInstCamData.micCameras.size() - 1) {
      mModelInstCamData.micSelectedCamera++;
    }
  }

  checkMouseEnable();
}

void VkRenderer::handleMouseButtonEvents(int button, int action, int mods) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT) {
      io.AddMouseButtonEvent(button, action == GLFW_PRESS);
    }

    /* hide from application if above ImGui window */
    if (io.WantCaptureMouse || io.WantTextInput) {
      return;
    }
  }

  /* trigger selection when left button has been released */
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE
    && mRenderData.rdApplicationMode == appMode::edit) {
    mMousePick = true;
    mSavedSelectedInstanceId = mModelInstCamData.micSelectedInstance;
  }

  /* move instance around with middle button pressed */
  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS
    && mRenderData.rdApplicationMode == appMode::edit) {
    mMouseMove = true;
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_LEFT_SHIFT;
      mMouseMoveVertical = true;
    }
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_RIGHT_SHIFT;
      mMouseMoveVertical = true;
    }

    if (mModelInstCamData.micSelectedInstance > 0) {
      mSavedInstanceSettings = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance)->getInstanceSettings();
    }
  }

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE
    && mRenderData.rdApplicationMode == appMode::edit) {
    mMouseMove = false;
    if (mModelInstCamData.micSelectedInstance > 0) {
      std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);
      InstanceSettings settings = instance->getInstanceSettings();
      mModelInstCamData.micSettingsContainer->applyEditInstanceSettings(instance, settings, mSavedInstanceSettings);
      setConfigDirtyFlag(true);
    }
  }

  std::shared_ptr<Camera> camera = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
  CameraSettings camSettings = camera->getCameraSettings();

  /* mouse camera movement only in edit mode, or with a free cam in view mode  */
  if (mRenderData.rdApplicationMode == appMode::edit ||
    (mRenderData.rdApplicationMode == appMode::view && camSettings.csCamType == cameraType::free)) {
    /* move camera view while right button is hold   */
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
      mMouseLock = true;
      mSavedCameraSettings = camSettings;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
      mMouseLock = false;
      mModelInstCamData.micSettingsContainer->applyEditCameraSettings(camera, camSettings, mSavedCameraSettings);
      setConfigDirtyFlag(true);
    }
  }

  checkMouseEnable();
}

void VkRenderer::handleMousePositionEvents(double xPos, double yPos) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent((float)xPos, (float)yPos);

    /* hide from application if above ImGui window */
    if (io.WantCaptureMouse || io.WantTextInput) {
      return;
    }
  }

  /* calculate relative movement from last position */
  int mouseMoveRelX = static_cast<int>(xPos) - mMouseXPos;
  int mouseMoveRelY = static_cast<int>(yPos) - mMouseYPos;

  std::shared_ptr<Camera> cam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
  CameraSettings camSettings = cam->getCameraSettings();

  if (mMouseLock) {
    camSettings.csViewAzimuth += mouseMoveRelX / 10.0f;
    /* keep between 0 and 360 degree */
    if (camSettings.csViewAzimuth < 0.0f) {
      camSettings.csViewAzimuth += 360.0f;
    }
    if (camSettings.csViewAzimuth >= 360.0f) {
      camSettings.csViewAzimuth -= 360.0f;
    }

    camSettings.csViewElevation -= mouseMoveRelY / 10.0f;
    /* keep between -89 and +89 degree */
    camSettings.csViewElevation =  std::clamp(camSettings.csViewElevation, -89.0f, 89.0f);
  }

  cam->setCameraSettings(camSettings);
  std::shared_ptr<AssimpInstance> currentInstance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);
  /* instance rotation with mouse */
  if (mRenderData.rdApplicationMode != appMode::edit) {
    if (mModelInstCamData.micSelectedInstance > 0) {
      float mouseXScaled = mouseMoveRelX / 10.0f;

      /* XXX: let user look up and down in first-person? */
      currentInstance->rotateInstance(mouseXScaled);
    }
  }

  if (mMouseMove) {
    if (mModelInstCamData.micSelectedInstance != 0) {

      float mouseXScaled = mouseMoveRelX / 20.0f;
      float mouseYScaled = mouseMoveRelY / 20.0f;
      float sinAzimuth = std::sin(glm::radians(camSettings.csViewAzimuth));
      float cosAzimuth = std::cos(glm::radians(camSettings.csViewAzimuth));

      float modelDistance = glm::length(camSettings.csWorldPosition - currentInstance->getWorldPosition()) / 50.0f;

      /* avoid breaking camera pos on model world position the logic in first-person camera */
      if (camSettings.csCamType == cameraType::firstPerson) {
        modelDistance = 0.1f;
      }

      glm::vec3 instancePos = currentInstance->getWorldPosition();
      glm::vec3 instanceRot = currentInstance->getRotation();
      float instanceScale = currentInstance->getScale();

      if (mMouseMoveVertical) {
        switch(mRenderData.rdInstanceEditMode) {
          case instanceEditMode::move:
            instancePos.y -= mouseYScaled * modelDistance;
            currentInstance->setWorldPosition(instancePos);
            break;
          case instanceEditMode::rotate:
            instanceRot.y -= mouseXScaled * 5.0f;
            currentInstance->rotateInstance(instanceRot);
            break;
          case instanceEditMode::scale:
            /* uniform scale, do nothing here  */
            break;
        }
      } else {
        switch(mRenderData.rdInstanceEditMode) {
          case instanceEditMode::move:
            instancePos.x += mouseXScaled * modelDistance * cosAzimuth - mouseYScaled * modelDistance * sinAzimuth;
            instancePos.z += mouseXScaled * modelDistance * sinAzimuth + mouseYScaled * modelDistance * cosAzimuth;
            currentInstance->setWorldPosition(instancePos);
            break;
          case instanceEditMode::rotate:
            instanceRot.z -= (mouseXScaled * cosAzimuth - mouseYScaled * sinAzimuth) * 5.0f;
            instanceRot.x += (mouseXScaled * sinAzimuth + mouseYScaled * cosAzimuth) * 5.0f;
            currentInstance->rotateInstance(instanceRot);
            break;
          case instanceEditMode::scale:
            instanceScale -= mouseYScaled / 2.0f;
            instanceScale= std::max(0.001f, instanceScale);
            currentInstance->setScale(instanceScale);
            break;
        }
      }
    }
  }

  /* save old values */
  mMouseXPos = static_cast<int>(xPos);
  mMouseYPos = static_cast<int>(yPos);
}

void VkRenderer::handleMouseWheelEvents(double xOffset, double yOffset) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMouseWheelEvent((float)xOffset, (float)yOffset);

    /* hide from application if above ImGui window */
    if (io.WantCaptureMouse || io.WantTextInput) {
      return;
    }
  }

  if (mRenderData.rdApplicationMode == appMode::edit) {
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      mMouseWheelScaleShiftKey = GLFW_KEY_LEFT_SHIFT;
      mMouseWheelScale = 4.0f;
    }
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      mMouseWheelScaleShiftKey = GLFW_KEY_RIGHT_SHIFT;
      mMouseWheelScale = 4.0f;
    }

    if  (glfwGetKey(mRenderData.rdWindow, mMouseWheelScaleShiftKey) == GLFW_RELEASE) {
      mMouseWheelScaleShiftKey = 0;
      mMouseWheelScale = 1.0f;
    }

    /* save timestamp of last scroll activity to check of scroll inactivity */
    mMouseWheelScrolling = true;
    mMouseWheelLastScrollTime = std::chrono::steady_clock::now();

    std::shared_ptr<Camera> cam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
    CameraSettings camSettings = cam->getCameraSettings();
    mSavedCameraWheelSettings = camSettings;

    if (camSettings.csCamProjection == cameraProjection::perspective) {
      int fieldOfView = camSettings.csFieldOfView - yOffset * mMouseWheelScale;
      fieldOfView = std::clamp(fieldOfView, 40, 100);
      camSettings.csFieldOfView = fieldOfView;
    } else {
      float orthoScale = camSettings.csOrthoScale - yOffset * mMouseWheelScale;
      orthoScale = std::clamp(orthoScale, 1.0f, 50.0f);
      camSettings.csOrthoScale = orthoScale;
    }
    cam->setCameraSettings(camSettings);
  }
}

void VkRenderer::handleMovementKeys() {
  mRenderData.rdMoveForward = 0;
  mRenderData.rdMoveRight = 0;
  mRenderData.rdMoveUp = 0;

  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();

    /* hide from application if above ImGui window */
    if (io.WantCaptureKeyboard || io.WantTextInput) {
      return;
    }
  }

  /* do not accept input whenever any dialog request comes in */
  if (mRenderData.rdRequestApplicationExit || mRenderData.rdNewConfigRequest ||
    mRenderData.rdLoadConfigRequest || mRenderData.rdSaveConfigRequest) {
    return;
  }

  /* camera movement */
  std::shared_ptr<Camera> cam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
  CameraSettings camSettings = cam->getCameraSettings();
  if (mRenderData.rdApplicationMode == appMode::edit ||
      (mRenderData.rdApplicationMode == appMode::view && camSettings.csCamType == cameraType::free)) {
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_W) == GLFW_PRESS) {
      mRenderData.rdMoveForward += 4;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_S) == GLFW_PRESS) {
      mRenderData.rdMoveForward -= 4;
    }

    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_A) == GLFW_PRESS) {
      mRenderData.rdMoveRight -= 4;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_D) == GLFW_PRESS) {
      mRenderData.rdMoveRight += 4;
    }

    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_E) == GLFW_PRESS) {
      mRenderData.rdMoveUp += 4;
    }
    if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Q) == GLFW_PRESS) {
      mRenderData.rdMoveUp -= 4;
    }

    /* speed up movement with shift */
    if ((glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
        (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
      mRenderData.rdMoveForward *= 5;
      mRenderData.rdMoveRight *= 5;
      mRenderData.rdMoveUp *= 5;
    }
  }

  /* instance movement */
  std::shared_ptr<AssimpInstance> currentInstance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);

  if (mRenderData.rdApplicationMode != appMode::edit && camSettings.csCamType != cameraType::free) {
    if (mModelInstCamData.micSelectedInstance > 0) {
      /* reset state to idle in every frame first */
      moveState state = moveState::idle;
      moveState nextState = moveState::idle;
      moveDirection dir = moveDirection::none;

      /* then check for movement and actions */
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_A) == GLFW_PRESS) {
        state = moveState::walk;
        dir |= moveDirection::left;
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_D) == GLFW_PRESS) {
        state = moveState::walk;
        dir |= moveDirection::right;
      }

      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_W) == GLFW_PRESS) {
        dir |= moveDirection::forward;
        state = moveState::walk;
        if ((glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
          (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
          /* only run forward in double speed */
          state = moveState::run;
          }
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_S) == GLFW_PRESS) {
        state = moveState::walk;
        dir |= moveDirection::back;
      }
      currentInstance->updateInstanceState(state, dir);

      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_R) == GLFW_PRESS) {
        nextState = moveState::roll;
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_E) == GLFW_PRESS) {
        nextState = moveState::punch;
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Q) == GLFW_PRESS) {
        nextState = moveState::kick;
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_F) == GLFW_PRESS) {
        nextState = moveState::wave;
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_U) == GLFW_PRESS) {
        nextState = moveState::interact;
        if (mRenderData.rdInteractWithInstanceId > 0) {
          mBehavior->addEvent(mRenderData.rdInteractWithInstanceId, nodeEvent::interaction);
        }
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_P) == GLFW_PRESS) {
        nextState = moveState::pick;
      }
      if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (state == moveState::walk || state == moveState::run) {
          nextState = moveState::jump;
        } else {
          nextState = moveState::hop;
        }
      }
      currentInstance->setNextInstanceState(nextState);
    }
  }
}

bool VkRenderer::createAABBLookup(std::shared_ptr<AssimpModel> model) {
  const int LOOKUP_SIZE = 1023;

  /* we use a single instance per clip */
  size_t numberOfClips = model->getAnimClips().size();
  size_t numberOfBones = model->getBoneList().size();

  /* we need valid model with triangles and animations */
  if (numberOfClips > 0 && numberOfBones > 0 && model->getTriangleCount() > 0) {
    Logger::log(1, "%s: playing animations for model %s\n", __FUNCTION__, model->getModelFileName().c_str());

    size_t trsMatrixSize = LOOKUP_SIZE * numberOfClips * numberOfBones * sizeof(glm::mat4);

    mPerInstanceAnimData.clear();
    mPerInstanceAnimData.resize(LOOKUP_SIZE * numberOfClips);

    /* play all animation steps */
    size_t clipToStore = 0;
    float timeScaleFactor = model->getMaxClipDuration() / static_cast<float>(LOOKUP_SIZE);
    for (int lookups = 0; lookups < LOOKUP_SIZE; ++lookups) {
      for (size_t i = 0; i < numberOfClips; ++i) {

        PerInstanceAnimData animData{};
        animData.firstAnimClipNum = i;
        animData.secondAnimClipNum = 0;
        animData.firstClipReplayTimestamp = lookups * timeScaleFactor;
        animData.secondClipReplayTimestamp = 0.0f;
        animData.blendFactor = 0.0f;

        mPerInstanceAnimData.at(clipToStore + i) = animData;
      }
      clipToStore += numberOfClips;
    }

    /* we need to update descriptors after the upload if buffer size changed */
    bool doComputeDescriptorUpdates = false;
    if (mPerInstanceAnimDataBuffer.bufferSize != LOOKUP_SIZE * numberOfClips * sizeof(PerInstanceAnimData) ||
        mShaderTRSMatrixBuffer.bufferSize != trsMatrixSize ||
        mShaderBoneMatrixBuffer.bufferSize != trsMatrixSize) {
      doComputeDescriptorUpdates = true;
    }

    mUploadToUBOTimer.start();
    ShaderStorageBuffer::uploadData(mRenderData, mPerInstanceAnimDataBuffer, mPerInstanceAnimData);
    mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

    /* resize SSBO if needed */
    ShaderStorageBuffer::checkForResize(mRenderData, mShaderBoneMatrixBuffer, trsMatrixSize);
    ShaderStorageBuffer::checkForResize(mRenderData, mShaderTRSMatrixBuffer, trsMatrixSize);

    if (doComputeDescriptorUpdates) {
      updateComputeDescriptorSets();
    }

    /* record compute commands */
    VkResult result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: compute fence reset failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    if (!CommandBuffer::reset(mRenderData.rdComputeCommandBuffer, 0)) {
      Logger::log(1, "%s error: failed to reset compute command buffer\n", __FUNCTION__);
      return false;
    }

    if (!CommandBuffer::beginSingleShot(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to begin compute command buffer\n", __FUNCTION__);
      return false;
    }

    /* do all calculations at once... may be undefined behavior */
    // runComputeShaders(model, numberOfClips * LOOKUP_SIZE, 0, 0, true);

    uint32_t computeShaderClipOffset = 0;
    uint32_t computeShaderInstanceOffset = 0;
    for (int lookups = 0; lookups < LOOKUP_SIZE; ++lookups) {
      runComputeShaders(model, numberOfClips, computeShaderClipOffset, computeShaderInstanceOffset, true);

      computeShaderClipOffset += numberOfClips * numberOfBones;
      computeShaderInstanceOffset += numberOfClips;
    }

    if (!CommandBuffer::end(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to end compute command buffer\n", __FUNCTION__);
      return false;
    }

    /* submit compute commands */
    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &mRenderData.rdComputeCommandBuffer;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };

    /* we must wait for the compute shaders to finish before we can read the bone data */
    result = vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: waiting for compute fence failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    /* extract bone matrix from SSBO */
    std::vector<glm::mat4> boneMatrix = ShaderStorageBuffer::getSsboDataMat4(mRenderData, mShaderBoneMatrixBuffer);

    /* our axis aligned bounding box */
    AABB aabb;

    std::vector<std::vector<AABB>> aabbLookups;
    aabbLookups.resize(numberOfClips);

    /* some models have a scaling set here... */
    glm::mat4 rootTransformMat = glm::transpose(model->getRootTranformationMatrix());

    /* and loop over clips and bones */
    size_t offset = 0;
    for (int lookups = 0; lookups < LOOKUP_SIZE; ++lookups) {
      for (size_t i = 0; i < numberOfClips; ++i) {
        /* add first point */
        glm::vec3 bonePos = (rootTransformMat * boneMatrix.at(offset + numberOfBones * i))[3];
        aabb.create(bonePos);

        /* extend AABB for other points */
        for (size_t j = 1; j < numberOfBones; ++j) {
          /* Shader:  uint index = node + numberOfBones * instance; */
          glm::vec3 bonePos = (rootTransformMat * boneMatrix.at(offset + numberOfBones * i + j))[3];
          aabb.addPoint(bonePos);
        }

        /* add all animation frames for the current clip */
        aabbLookups.at(i).emplace_back(aabb);
      }
      offset += numberOfClips * numberOfBones;
    }

    model->setAABBLookup(aabbLookups);
  }

  return true;
}

bool VkRenderer::checkForInstanceCollisions() {
  /* get bounding box intersections */
  mModelInstCamData.micInstanceCollisions = mQuadtree->findAllIntersections();

  if (mRenderData.rdCheckCollisions == collisionChecks::boundingSpheres) {
    mBoundingSpheresPerInstance.clear();

    /* calculate collision spheres per model */
    std::map<std::string, std::set<int>> modelToInstanceMapping;

    for (const auto& instancePairs : mModelInstCamData.micInstanceCollisions) {
      modelToInstanceMapping[mModelInstCamData.micAssimpInstances.at(instancePairs.first)->getModel()->getModelFileName()].insert(instancePairs.first);
      modelToInstanceMapping[mModelInstCamData.micAssimpInstances.at(instancePairs.second)->getModel()->getModelFileName()].insert(instancePairs.second);
    }

    /* count total number of spheres to calculate */
    int totalSpheres = 0;
    for (const auto& collisionInstances : modelToInstanceMapping) {
      std::shared_ptr<AssimpModel> model = getModel(collisionInstances.first);
      if (!model->hasAnimations()) {
        continue;
      }

      std::string modelName = model->getModelFileName();
      std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[modelName];

      size_t numberOfBones = model->getBoneList().size();
      size_t numInstances = instances.size();

      size_t numberOfSpheres = numInstances * numberOfBones;

      totalSpheres += numberOfSpheres;
    }

    bool doSphereDescriptorUpdates = false;
    if (mBoundingSphereBuffer.bufferSize != totalSpheres * sizeof(glm::vec4)) {
      doSphereDescriptorUpdates = true;
    }

    /* resize SSBO if needed */
    ShaderStorageBuffer::checkForResize(mRenderData, mBoundingSphereBuffer, totalSpheres * sizeof(glm::vec4));

    if (doSphereDescriptorUpdates) {
      updateSphereComputeDescriptorSets();
    }

    int sphereModelOffset = 0;
    for (const auto& collisionInstances : modelToInstanceMapping) {
      std::shared_ptr<AssimpModel> model = getModel(collisionInstances.first);
      if (!model->hasAnimations()) {
        continue;
      }

      size_t numInstances = collisionInstances.second.size();
      std::vector<int> instanceIds = std::vector(collisionInstances.second.begin(), collisionInstances.second.end());

      size_t numberOfBones = model->getBoneList().size();

      size_t numberOfSpheres = numInstances * numberOfBones;
      size_t trsMatrixSize = numInstances * numberOfBones * sizeof(glm::mat4);

      /* Vulkan needs separate buffers */
      mSphereWorldPosMatrices.clear();
      mSphereWorldPosMatrices.resize(numInstances);

      mSpherePerInstanceAnimData.clear();
      mSpherePerInstanceAnimData.resize(numInstances);

      for (size_t i = 0; i < numInstances; ++i) {
        InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getInstanceSettings();

        PerInstanceAnimData animData{};
        animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
        animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
        animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
        animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
        animData.blendFactor = instSettings.isAnimBlendFactor;

        mSpherePerInstanceAnimData.at(i) = animData;

        mSphereWorldPosMatrices.at(i) = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getWorldTransformMatrix();
      }

      /* we need to update descriptors after the upload if buffer size changed */
      bool doComputeDescriptorUpdates = false;
      if (mSphereModelRootMatrixBuffer.bufferSize != numInstances * sizeof(glm::mat4) ||
          mSpherePerInstanceAnimDataBuffer.bufferSize != numInstances * sizeof(PerInstanceAnimData) ||
          mSphereTRSMatrixBuffer.bufferSize != trsMatrixSize ||
          mSphereBoneMatrixBuffer.bufferSize != trsMatrixSize) {
        doComputeDescriptorUpdates = true;
      }

      mUploadToUBOTimer.start();
      ShaderStorageBuffer::uploadData(mRenderData, mSpherePerInstanceAnimDataBuffer, mSpherePerInstanceAnimData);
      ShaderStorageBuffer::uploadData(mRenderData, mSphereModelRootMatrixBuffer, mSphereWorldPosMatrices);
      mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

      /* resize SSBO if needed */
      ShaderStorageBuffer::checkForResize(mRenderData, mSphereBoneMatrixBuffer, trsMatrixSize);
      ShaderStorageBuffer::checkForResize(mRenderData, mSphereTRSMatrixBuffer, trsMatrixSize);

      if (doComputeDescriptorUpdates) {
        updateSphereComputeDescriptorSets();
      }

      /* in case data was changed */
      model->updateBoundingSphereAdjustments(mRenderData);

      /* record compute commands */
      VkResult result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence);
      if (result != VK_SUCCESS) {
        Logger::log(1, "%s error: compute fence reset failed (error: %i)\n", __FUNCTION__, result);
        return false;
      }

      if (!CommandBuffer::reset(mRenderData.rdComputeCommandBuffer, 0)) {
        Logger::log(1, "%s error: failed to reset compute command buffer\n", __FUNCTION__);
        return false;
      }

      if (!CommandBuffer::beginSingleShot(mRenderData.rdComputeCommandBuffer)) {
        Logger::log(1, "%s error: failed to begin compute command buffer\n", __FUNCTION__);
        return false;
      }

      runBoundingSphereComputeShaders(model, numInstances, sphereModelOffset);
      sphereModelOffset += numberOfSpheres;

      if (!CommandBuffer::end(mRenderData.rdComputeCommandBuffer)) {
        Logger::log(1, "%s error: failed to end compute command buffer\n", __FUNCTION__);
        return false;
      }

      /* submit compute commands */
      VkSubmitInfo computeSubmitInfo{};
      computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      computeSubmitInfo.commandBufferCount = 1;
      computeSubmitInfo.pCommandBuffers = &mRenderData.rdComputeCommandBuffer;

      result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
      if (result != VK_SUCCESS) {
        Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
        return false;
      };

      /* we must wait for the compute shaders to finish before we can read the bone data */
      result = vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence, VK_TRUE, UINT64_MAX);
      if (result != VK_SUCCESS) {
        Logger::log(1, "%s error: waiting for compute fence failed (error: %i)\n", __FUNCTION__, result);
        return false;
      }

    }

    /* read sphere SSBO */
    std::vector<glm::vec4> boundingSpheres = ShaderStorageBuffer::getSsboDataVec4(mRenderData, mBoundingSphereBuffer, totalSpheres);

    sphereModelOffset = 0;
    for (const auto& collisionInstances : modelToInstanceMapping) {
      std::shared_ptr<AssimpModel> model = getModel(collisionInstances.first);
      if (!model->hasAnimations()) {
        continue;
      }

      size_t numInstances = collisionInstances.second.size();
      std::vector<int> instanceIds = std::vector(collisionInstances.second.begin(), collisionInstances.second.end());

      size_t numberOfBones = model->getBoneList().size();
      size_t numberOfSpheres = numInstances * numberOfBones;

      for (size_t i = 0; i < numInstances; ++i) {
        InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getInstanceSettings();
        int instanceIndex = instSettings.isInstanceIndexPosition;
        mBoundingSpheresPerInstance[instanceIndex].resize(numberOfBones);

        std::copy(boundingSpheres.begin() + sphereModelOffset + i * numberOfBones,
          boundingSpheres.begin() + sphereModelOffset + (i + 1) * numberOfBones,
          mBoundingSpheresPerInstance[instanceIndex].begin());
      }
      sphereModelOffset += numberOfSpheres;
    }

    checkForBoundingSphereCollisions();
  }

  /* get (possibly cleaned) number of collisions */
  mRenderData.rdNumberOfCollisions = mModelInstCamData.micInstanceCollisions.size();

  if (mRenderData.rdCheckCollisions != collisionChecks::none) {
    reactToInstanceCollisions();
  }
  return true;
}

void VkRenderer::checkForBorderCollisions() {
  for (const auto& instancesPerModel : mModelInstCamData.micAssimpInstancesPerModel) {
    std::shared_ptr<AssimpModel> model = getModel(instancesPerModel.first);
    /* non-animated models have no lookup data */
    if (!model || !model->hasAnimations()) {
      continue;
    }

    std::vector<std::shared_ptr<AssimpInstance>> instances = instancesPerModel.second;
    for (size_t i = 0; i < instances.size(); ++i) {
      InstanceSettings instSettings = instances.at(i)->getInstanceSettings();

      /* check world borders */
      AABB instanceAABB = model->getAABB(instSettings);
      glm::vec3 minPos = instanceAABB.getMinPos();
      glm::vec3 maxPos = instanceAABB.getMaxPos();
      if (minPos.x < mWorldBoundaries->getTopLeft().x || maxPos.x > mWorldBoundaries->getRight() ||
        minPos.z < mWorldBoundaries->getTopLeft().y || maxPos.z > mWorldBoundaries->getBottom()) {
        mModelInstCamData.micNodeEventCallbackFunction(instSettings.isInstanceIndexPosition, nodeEvent::instanceToEdgeCollision);
        }
    }
  }
}

void VkRenderer::checkForBoundingSphereCollisions() {
  std::set<std::pair<int, int>> sphereCollisions{};

  for (const auto& instancePairs : mModelInstCamData.micInstanceCollisions) {
    int firstId = instancePairs.first;
    int secondId = instancePairs.second;

    /* brute force check of sphere vs sphere */
    bool collisionDetected = false;

    for (size_t first = 0; first < mBoundingSpheresPerInstance[firstId].size(); ++first) {
      glm::vec4 firstSphereData = mBoundingSpheresPerInstance[firstId].at(first);
      float firstRadius = firstSphereData.w;

      /* no need to check disabled spheres */
      if (firstRadius == 0.0f) {
        continue;
      }

      glm::vec3 firstSpherePos = glm::vec3(firstSphereData.x, firstSphereData.y, firstSphereData.z);

      for (size_t second = 0; second < mBoundingSpheresPerInstance[secondId].size(); ++second) {
        glm::vec4 secondSphereData = mBoundingSpheresPerInstance[secondId].at(second);
        float secondRadius = secondSphereData.w;

        /* no need to check disabled spheres */
        if (secondRadius == 0.0f) {
          continue;
        }

        glm::vec3 secondSpherePos = glm::vec3(secondSphereData.x, secondSphereData.y, secondSphereData.z);

        /* check for intersections  */
        glm::vec3 centerDistance = firstSpherePos - secondSpherePos;
        float centerDistanceSquared = glm::dot(centerDistance, centerDistance);

        float sphereRadiusSum = firstRadius + secondRadius;
        float sphereRadiusSumSquared = sphereRadiusSum * sphereRadiusSum;

        /* flag as a hit and exit immediately */
        if (centerDistanceSquared <= sphereRadiusSumSquared) {
          collisionDetected = true;
          break;
        }
      }

      if (collisionDetected) {
        break;
      }
    }

    /* store collisions in set */
    if (collisionDetected) {
      sphereCollisions.insert({firstId, secondId});
    }
  }

  /* replace collided instance data with new ones */
  mModelInstCamData.micInstanceCollisions.clear();
  mModelInstCamData.micInstanceCollisions.insert(sphereCollisions.begin(), sphereCollisions.end());
}

void VkRenderer::reactToInstanceCollisions() {
  std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstances;

  for (const auto& instancePairs : mModelInstCamData.micInstanceCollisions) {
    mModelInstCamData.micNodeEventCallbackFunction(
      instances.at(instancePairs.first)->getInstanceSettings().isInstanceIndexPosition,
      nodeEvent::instanceToInstanceCollision);
    mModelInstCamData.micNodeEventCallbackFunction(
      instances.at(instancePairs.second)->getInstanceSettings().isInstanceIndexPosition,
      nodeEvent::instanceToInstanceCollision);
  }
}

void VkRenderer::runComputeShaders(std::shared_ptr<AssimpModel> model, int numInstances,
    uint32_t modelOffset, uint32_t instanceOffset, bool useEmptyBoneOffsets) {
  uint32_t numberOfBones = model->getBoneList().size();

  /* node transformation */
  if (model->hasHeadMovementAnimationsMapped()) {
    vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
      mRenderData.rdAssimpComputeHeadMoveTransformPipeline);
  } else {
    vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
      mRenderData.rdAssimpComputeTransformPipeline);
  }

  VkDescriptorSet &modelTransformDescriptorSet = model->getTransformDescriptorSet();
  std::vector<VkDescriptorSet> transformComputeSets = { mRenderData.rdAssimpComputeTransformDescriptorSet, modelTransformDescriptorSet };

  vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeTransformaPipelineLayout, 0, static_cast<uint32_t>(transformComputeSets.size()), transformComputeSets.data(), 0, 0);

  mUploadToUBOTimer.start();
  mComputeModelData.pkModelOffset = modelOffset;
  mComputeModelData.pkInstanceOffset = instanceOffset;
  vkCmdPushConstants(mRenderData.rdComputeCommandBuffer, mRenderData.rdAssimpComputeTransformaPipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(VkComputePushConstants)), &mComputeModelData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  vkCmdDispatch(mRenderData.rdComputeCommandBuffer, numberOfBones, static_cast<uint32_t>(std::ceil(numInstances / 32.0f)), 1);

  /* memroy barrier between the compute shaders
   * wait for TRS buffer to be written  */
  VkMemoryBarrier trsBufferBarrier{};
  trsBufferBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  trsBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  trsBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
    &trsBufferBarrier, 0, nullptr, 0, nullptr);

  /* matrix multiplication */
  vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeMatrixMultPipeline);

  if (useEmptyBoneOffsets) {
    VkDescriptorSet &modelMatrixMultDescriptorSet = model->getMatrixMultEmptyOffsetDescriptorSet();
    std::vector<VkDescriptorSet> matrixMultComputeSets = { mRenderData.rdAssimpComputeMatrixMultDescriptorSet, modelMatrixMultDescriptorSet };

    vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
      mRenderData.rdAssimpComputeMatrixMultPipelineLayout, 0, static_cast<uint32_t>(matrixMultComputeSets.size()), matrixMultComputeSets.data(), 0, 0);
  } else {
    VkDescriptorSet &modelMatrixMultDescriptorSet = model->getMatrixMultDescriptorSet();
    std::vector<VkDescriptorSet> matrixMultComputeSets = { mRenderData.rdAssimpComputeMatrixMultDescriptorSet, modelMatrixMultDescriptorSet };

    vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
      mRenderData.rdAssimpComputeMatrixMultPipelineLayout, 0, static_cast<uint32_t>(matrixMultComputeSets.size()), matrixMultComputeSets.data(), 0, 0);
  }

  mUploadToUBOTimer.start();
  mComputeModelData.pkModelOffset = modelOffset;
  vkCmdPushConstants(mRenderData.rdComputeCommandBuffer, mRenderData.rdAssimpComputeMatrixMultPipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(VkComputePushConstants)), &mComputeModelData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  vkCmdDispatch(mRenderData.rdComputeCommandBuffer, static_cast<uint32_t>(numberOfBones), static_cast<uint32_t>(std::ceil(numInstances / 32.0f)), 1);

  /* memroy barrier after compute shader
   * wait for bone matrix buffer to be written  */
  VkMemoryBarrier boneMatrixBufferBarrier{};
  boneMatrixBufferBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  boneMatrixBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  boneMatrixBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
    &boneMatrixBufferBarrier, 0, nullptr, 0, nullptr);
}

void VkRenderer::runBoundingSphereComputeShaders(std::shared_ptr<AssimpModel> model, int numInstances,
    uint32_t modelOffset) {
  uint32_t numberOfBones = model->getBoneList().size();

  /* node transformation */
  vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeTransformPipeline);

  VkDescriptorSet &modelTransformDescriptorSet = model->getTransformDescriptorSet();
  std::vector<VkDescriptorSet> transformComputeSets = { mRenderData.rdAssimpComputeSphereTransformDescriptorSet, modelTransformDescriptorSet };

  vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeTransformaPipelineLayout, 0, static_cast<uint32_t>(transformComputeSets.size()), transformComputeSets.data(), 0, 0);

  mUploadToUBOTimer.start();
  mComputeModelData.pkModelOffset = 0;
  mComputeModelData.pkInstanceOffset = 0;
  vkCmdPushConstants(mRenderData.rdComputeCommandBuffer, mRenderData.rdAssimpComputeTransformaPipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(VkComputePushConstants)), &mComputeModelData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  vkCmdDispatch(mRenderData.rdComputeCommandBuffer, numberOfBones, static_cast<uint32_t>(std::ceil(numInstances / 32.0f)), 1);

  /* memroy barrier between the compute shaders
   * wait for TRS buffer to be written  */
  VkMemoryBarrier trsBufferBarrier{};
  trsBufferBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  trsBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  trsBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
    &trsBufferBarrier, 0, nullptr, 0, nullptr);

  /* matrix multiplication */
  vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeMatrixMultPipeline);

  VkDescriptorSet &modelMatrixMultDescriptorSet = model->getMatrixMultEmptyOffsetDescriptorSet();
  std::vector<VkDescriptorSet> matrixMultComputeSets = { mRenderData.rdAssimpComputeSphereMatrixMultDescriptorSet, modelMatrixMultDescriptorSet };

  vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeMatrixMultPipelineLayout, 0, static_cast<uint32_t>(matrixMultComputeSets.size()), matrixMultComputeSets.data(), 0, 0);

  mUploadToUBOTimer.start();
  mComputeModelData.pkModelOffset = 0;
  mComputeModelData.pkInstanceOffset = 0;
  vkCmdPushConstants(mRenderData.rdComputeCommandBuffer, mRenderData.rdAssimpComputeMatrixMultPipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(VkComputePushConstants)), &mComputeModelData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  vkCmdDispatch(mRenderData.rdComputeCommandBuffer, numberOfBones, static_cast<uint32_t>(std::ceil(numInstances / 32.0f)), 1);

  /* memroy barrier after compute shader
   * wait for bone matrix buffer to be written  */
  VkMemoryBarrier boneMatrixBufferBarrier{};
  boneMatrixBufferBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  boneMatrixBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  boneMatrixBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
    &boneMatrixBufferBarrier, 0, nullptr, 0, nullptr);

  vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    mRenderData.rdAssimpComputeBoundingSpheresPipeline);

  VkDescriptorSet &boundingSpheresDescriptorSet = model->getBoundingSphereDescriptorSet();
  std::vector<VkDescriptorSet> boundingSphereComputeSets = { mRenderData.rdAssimpComputeBoundingSpheresDescriptorSet, boundingSpheresDescriptorSet };

  vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeBoundingSpheresPipelineLayout, 0, static_cast<uint32_t>(boundingSphereComputeSets.size()), boundingSphereComputeSets.data(), 0, 0);

  mUploadToUBOTimer.start();
  mComputeModelData.pkModelOffset = modelOffset;
  mComputeModelData.pkInstanceOffset = 0;
  vkCmdPushConstants(mRenderData.rdComputeCommandBuffer, mRenderData.rdAssimpComputeBoundingSpheresPipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(VkComputePushConstants)), &mComputeModelData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  vkCmdDispatch(mRenderData.rdComputeCommandBuffer, numberOfBones, static_cast<uint32_t>(std::ceil(numInstances / 32.0f)), 1);

  /* memroy barrier between the compute shaders
   * wait for bounding sphere buffer to be written  */
  VkMemoryBarrier boundingSphereBufferBarrier{};
  boundingSphereBufferBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  boundingSphereBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  boundingSphereBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
    &boundingSphereBufferBarrier, 0, nullptr, 0, nullptr);
}

void VkRenderer::findInteractionInstances() {
  if (!mRenderData.rdInteraction) {
    return;
  }
  mRenderData.rdInteractionCandidates.clear();

  if (mModelInstCamData.micSelectedInstance == 0) {
    return;
  }
  std::shared_ptr<AssimpInstance> currentInstance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);
  InstanceSettings curInstSettings = currentInstance->getInstanceSettings();

  /* query quadtree with a bounding box */
  glm::vec3 instancePos = curInstSettings.isWorldPosition;
  glm::vec2 instancePos2D = glm::vec2(instancePos.x, instancePos.z);
  glm::vec2 querySize = glm::vec2(mRenderData.rdInteractionMaxRange);
  BoundingBox2D queryBox = BoundingBox2D(instancePos2D - querySize / 2.0f, querySize);

  std::set<int> queriedNearInstances = mQuadtree->query(queryBox);

  /* skip ourselve */
  queriedNearInstances.erase(curInstSettings.isInstanceIndexPosition);

  if (queriedNearInstances.size() == 0) {
    return;
  }

  std::set<int> nearInstances{};
  for (const auto& id : queriedNearInstances) {
    std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(id);
    InstanceSettings instSettings = instance->getInstanceSettings();

    float distance = glm::length(instSettings.isWorldPosition - curInstSettings.isWorldPosition);
    if (distance > mRenderData.rdInteractionMinRange) {
      nearInstances.emplace(id);
    }
  }

  if (nearInstances.size() == 0) {
    return;
  }

  mRenderData.rdNumberOfInteractionCandidates = nearInstances.size();

  if (mRenderData.rdDrawInteractionAABBs == interactionDebugDraw::distance) {
    mRenderData.rdInteractionCandidates = nearInstances;
  }

  std::set<int> instancesFacingToUs{};
  for (const auto& id : nearInstances) {
    std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(id);
    InstanceSettings instSettings = instance->getInstanceSettings();

    glm::vec3 distanceVector = glm::normalize(instSettings.isWorldPosition - curInstSettings.isWorldPosition);
    float angle = glm::degrees(glm::acos(glm::dot(currentInstance->get2DRotationVector(), distanceVector)));
    float instAngle = glm::degrees(glm::acos(glm::dot(instance->get2DRotationVector(), -distanceVector)));

    if (angle < mRenderData.rdInteractionFOV && instAngle < mRenderData.rdInteractionFOV)  {
      instancesFacingToUs.emplace(id);
    }
  }

  if (instancesFacingToUs.size() == 0) {
    return;
  }

  if (mRenderData.rdDrawInteractionAABBs == interactionDebugDraw::facingTowardsUs) {
    mRenderData.rdInteractionCandidates = instancesFacingToUs;
  }

  std::vector<std::pair<float, int>> sortedDistances;
  for (const auto& id : instancesFacingToUs) {
    std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(id);
    InstanceSettings instSettings = instance->getInstanceSettings();

    float distance = glm::length(instSettings.isWorldPosition - curInstSettings.isWorldPosition);
    sortedDistances.emplace_back(std::make_pair(distance, id));
  }

  std::sort(sortedDistances.begin(), sortedDistances.end());
  mRenderData.rdInteractWithInstanceId = sortedDistances.begin()->second;

  if (mRenderData.rdDrawInteractionAABBs == interactionDebugDraw::nearestCandidate) {
    mRenderData.rdInteractionCandidates = { mRenderData.rdInteractWithInstanceId };
  }
}

void VkRenderer::drawInteractionDebug() {
  if (mModelInstCamData.micSelectedInstance == 0) {
    return;
  }

  glm::vec4 aabbColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

  VkLineMesh InteractionMesh;
  VkLineVertex vertex;
  vertex.color = aabbColor;

  std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);
  InstanceSettings instSettings = instance->getInstanceSettings();

  if (mRenderData.rdDrawInteractionRange) {
    glm::vec3 instancePos = instSettings.isWorldPosition;
    glm::vec2 instancePos2D = glm::vec2(instancePos.x, instancePos.z);

    glm::vec2 minQueryBoxTopLeft = glm::vec2(instancePos2D) - glm::vec2(mRenderData.rdInteractionMinRange / 2.0f);
    glm::vec2 minQueryBoxBottomRight = glm::vec2(instancePos2D) + glm::vec2(mRenderData.rdInteractionMinRange / 2.0f);

    glm::vec2 maxQueryBoxTopLeft = glm::vec2(instancePos2D) - glm::vec2(mRenderData.rdInteractionMaxRange / 2.0f);
    glm::vec2 maxQueryBoxBottomRight = glm::vec2(instancePos2D) + glm::vec2(mRenderData.rdInteractionMaxRange / 2.0f);

    /* min range */
    vertex.position = glm::vec3(minQueryBoxTopLeft.x, instancePos.y, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxTopLeft.x, instancePos.y, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(minQueryBoxTopLeft.x, instancePos.y, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxBottomRight.x, instancePos.y, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(minQueryBoxBottomRight.x, instancePos.y, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxBottomRight.x, instancePos.y, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(minQueryBoxBottomRight.x, instancePos.y, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxTopLeft.x, instancePos.y, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);

    /* max range */
    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, instancePos.y, maxQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, instancePos.y, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, instancePos.y, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, instancePos.y, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, instancePos.y, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, instancePos.y, maxQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, instancePos.y, maxQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, instancePos.y, maxQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
  }

  /* draw FOV lines */
  if (mRenderData.rdDrawInteractionFOV) {
    std::set<int> drawFOVLines = mRenderData.rdInteractionCandidates;
    drawFOVLines.emplace(instSettings.isInstanceIndexPosition);

    for (const auto id : drawFOVLines) {
      std::shared_ptr<AssimpInstance> fovInstance = mModelInstCamData.micAssimpInstances.at(id);
      InstanceSettings fovInstSettings = fovInstance->getInstanceSettings();

      vertex.position = fovInstSettings.isWorldPosition;
      InteractionMesh.vertices.emplace_back(vertex);

      float minAngle = fovInstSettings.isWorldRotation.y - mRenderData.rdInteractionFOV;
      if (minAngle < -180.0f) {
        minAngle += 360.0f;
      }
      if (minAngle > 180.0f) {
        minAngle -= 360.0f;
      }
      float sinRot = std::sin(glm::radians(minAngle));
      float cosRot = std::cos(glm::radians(minAngle));
      vertex.position = fovInstSettings.isWorldPosition + glm::normalize(glm::vec3(sinRot, 0.0f, cosRot)) * 3.0f;
      InteractionMesh.vertices.emplace_back(vertex);

      vertex.position = fovInstSettings.isWorldPosition;
      InteractionMesh.vertices.emplace_back(vertex);

      float maxAngle = fovInstSettings.isWorldRotation.y + mRenderData.rdInteractionFOV;
      if (maxAngle < -180.0f) {
        maxAngle += 360.0f;
      }
      if (maxAngle > 180.0f) {
        maxAngle -= 360.0f;
      }
      sinRot = std::sin(glm::radians(maxAngle));
      cosRot = std::cos(glm::radians(maxAngle));
      vertex.position = fovInstSettings.isWorldPosition + glm::normalize(glm::vec3(sinRot, 0.0f, cosRot)) * 3.0f;
      InteractionMesh.vertices.emplace_back(vertex);
    }
  }

  mLineIndexCount += InteractionMesh.vertices.size();
  mLineMesh->vertices.insert(mLineMesh->vertices.end(), InteractionMesh.vertices.begin(), InteractionMesh.vertices.end());

  /* draw instance AABBs */
  if (mRenderData.rdInteractionCandidates.empty()) {
    return;
  }

  std::vector<std::shared_ptr<AssimpInstance>> instancesToDraw{};
  for (const int id : mRenderData.rdInteractionCandidates) {
    instancesToDraw.emplace_back(mModelInstCamData.micAssimpInstances.at(id));
  }

  drawAABBs(instancesToDraw, aabbColor);
}

void VkRenderer::drawAABBs(std::vector<std::shared_ptr<AssimpInstance>> instances, glm::vec4 aabbColor) {
  std::shared_ptr<VkLineMesh> aabbLineMesh = nullptr;;

  mAABBMesh->vertices.clear();
  AABB instanceAABB;
  mAABBMesh->vertices.resize(instances.size() * instanceAABB.getAABBLines(aabbColor)->vertices.size());

  for (size_t i = 0; i < instances.size(); ++i) {
    InstanceSettings instSettings = instances.at(i)->getInstanceSettings();

    /* skip null instance */
    if (instSettings.isInstanceIndexPosition == 0) {
      continue;
    }

    std::shared_ptr<AssimpModel> model = instances.at(i)->getModel();

    instanceAABB = model->getAABB(instSettings);
    aabbLineMesh = instanceAABB.getAABBLines(aabbColor);

    if (aabbLineMesh) {
      std::copy(aabbLineMesh->vertices.begin(), aabbLineMesh->vertices.end(),
      mAABBMesh->vertices.begin() + i * aabbLineMesh->vertices.size());
    }
  }

  mLineIndexCount += mAABBMesh->vertices.size();
  mLineMesh->vertices.insert(mLineMesh->vertices.end(), mAABBMesh->vertices.begin(), mAABBMesh->vertices.end());
}

void VkRenderer::drawCollisionDebug() {
  /* draw AABB lines and bounding sphere of selected instance */
  if (mRenderData.rdDrawCollisionAABBs == collisionDebugDraw::colliding ||
    mRenderData.rdDrawCollisionAABBs == collisionDebugDraw::all) {
    std::set<int> uniqueInstanceIds;

    for (const auto& colliding : mModelInstCamData.micInstanceCollisions) {
      uniqueInstanceIds.insert(colliding.first);
      uniqueInstanceIds.insert(colliding.second);
    }

    std::vector<std::shared_ptr<AssimpInstance>> instancestoDraw;
    glm::vec4 aabbColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    /* draw colliding instances in red */
    if (mRenderData.rdDrawCollisionAABBs == collisionDebugDraw::colliding ||
      mRenderData.rdDrawCollisionAABBs == collisionDebugDraw::all) {
      for (const auto id : uniqueInstanceIds) {
        instancestoDraw.push_back(mModelInstCamData.micAssimpInstances.at(id));
      }
      /* draw red lines for collisions */
      aabbColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
      drawAABBs(instancestoDraw, aabbColor);
    }

    /* draw yellow lines for non-colliiding instances */
    /* we can just overdraw the lines, thanks to the z-buffer the red lines stay :) */
    if (mRenderData.rdDrawCollisionAABBs == collisionDebugDraw::all) {
      instancestoDraw = mModelInstCamData.micAssimpInstances;
      aabbColor = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
      drawAABBs(instancestoDraw, aabbColor);
    }
  }
}

bool VkRenderer::createSelectedBoundingSpheres() {
  if (mModelInstCamData.micSelectedInstance > 0 ) {
    std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);
    std::shared_ptr<AssimpModel> model = instance->getModel();

    if (!model->hasAnimations()) {
      return false;
    }

    size_t numberOfBones = model->getBoneList().size();

    size_t numberOfSpheres = numberOfBones;
    size_t trsMatrixSize = numberOfBones * sizeof(glm::mat4);

    mSphereWorldPosMatrices.clear();
    mSphereWorldPosMatrices.resize(1);

    mSpherePerInstanceAnimData.clear();
    mSpherePerInstanceAnimData.resize(1);

    InstanceSettings instSettings = instance->getInstanceSettings();

    PerInstanceAnimData animData{};
    animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
    animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
    animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
    animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
    animData.blendFactor = instSettings.isAnimBlendFactor;

    mSpherePerInstanceAnimData.at(0) = animData;

    mSphereWorldPosMatrices.at(0) = instance->getWorldTransformMatrix();

    bool doComputeDescriptorUpdates = false;
    if (mSphereModelRootMatrixBuffer.bufferSize != sizeof(glm::mat4) ||
        mSpherePerInstanceAnimDataBuffer.bufferSize != sizeof(PerInstanceAnimData) ||
        mSphereTRSMatrixBuffer.bufferSize != trsMatrixSize ||
        mSphereBoneMatrixBuffer.bufferSize != trsMatrixSize ||
        mBoundingSphereBuffer.bufferSize != numberOfSpheres * sizeof(glm::vec4)) {
        doComputeDescriptorUpdates = true;
    }

    mUploadToUBOTimer.start();
    ShaderStorageBuffer::uploadData(mRenderData, mSpherePerInstanceAnimDataBuffer, mSpherePerInstanceAnimData);
    ShaderStorageBuffer::uploadData(mRenderData, mSphereModelRootMatrixBuffer, mSphereWorldPosMatrices);
    mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

    /* resize SSBO if needed */
    ShaderStorageBuffer::checkForResize(mRenderData, mSphereBoneMatrixBuffer, trsMatrixSize);
    ShaderStorageBuffer::checkForResize(mRenderData, mSphereTRSMatrixBuffer, trsMatrixSize);
    ShaderStorageBuffer::checkForResize(mRenderData, mBoundingSphereBuffer, numberOfSpheres * sizeof(glm::vec4));

    if (doComputeDescriptorUpdates) {
      updateSphereComputeDescriptorSets();
    }

    /* in case data was changed */
    model->updateBoundingSphereAdjustments(mRenderData);

    /* record compute commands */
    VkResult result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: compute fence reset failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    if (!CommandBuffer::reset(mRenderData.rdComputeCommandBuffer, 0)) {
      Logger::log(1, "%s error: failed to reset compute command buffer\n", __FUNCTION__);
      return false;
    }

    if (!CommandBuffer::beginSingleShot(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to begin compute command buffer\n", __FUNCTION__);
      return false;
    }

    runBoundingSphereComputeShaders(model, 1, 0);
    mCollidingSphereCount = numberOfSpheres;

    if (!CommandBuffer::end(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to end compute command buffer\n", __FUNCTION__);
      return false;
    }

    /* submit compute commands */
    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &mRenderData.rdComputeCommandBuffer;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };

    /* we must wait for the compute shaders to finish before we can read the bone data */
    result = vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: waiting for compute fence failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  if (mCollidingSphereCount > 0) {
    mUploadToVBOTimer.start();
    VertexBuffer::uploadData(mRenderData, mSphereVertexBuffer, mSphereMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();
  }

  return true;
}

bool VkRenderer::createCollidingBoundingSpheres() {
  /* split instances in models - use a std::set to get unique instance IDs */
  std::map<std::string, std::set<int>> modelToInstanceMapping;

  for (const auto& instancePairs : mModelInstCamData.micInstanceCollisions) {
    modelToInstanceMapping[mModelInstCamData.micAssimpInstances.at(instancePairs.first)->getModel()->getModelFileName()].insert(instancePairs.first);
    modelToInstanceMapping[mModelInstCamData.micAssimpInstances.at(instancePairs.second)->getModel()->getModelFileName()].insert(instancePairs.second);
  }

  int totalSpheres = 0;
  for (const auto& collisionInstances : modelToInstanceMapping) {
    std::shared_ptr<AssimpModel> model = getModel(collisionInstances.first);
    if (!model->hasAnimations()) {
      continue;
    }

    std::string modelName = model->getModelFileName();
    std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[modelName];

    size_t numberOfBones = model->getBoneList().size();
    size_t numInstances = instances.size();

    size_t numberOfSpheres = numInstances * numberOfBones;

    totalSpheres += numberOfSpheres;
  }

  bool doSphereDescriptorUpdates = false;
  if (mBoundingSphereBuffer.bufferSize != totalSpheres * sizeof(glm::vec4)) {
    doSphereDescriptorUpdates = true;
  }

  /* resize SSBO if needed */
  ShaderStorageBuffer::checkForResize(mRenderData, mBoundingSphereBuffer, totalSpheres * sizeof(glm::vec4));

  if (doSphereDescriptorUpdates) {
    updateSphereComputeDescriptorSets();
  }

  int sphereModelOffset = 0;
  for (const auto& collisionInstances : modelToInstanceMapping) {
    std::shared_ptr<AssimpModel> model = getModel(collisionInstances.first);
    if (!model->hasAnimations()) {
      continue;
    }

    size_t numInstances = collisionInstances.second.size();
    std::vector<int> instanceIds = std::vector(collisionInstances.second.begin(), collisionInstances.second.end());

    size_t numberOfBones = model->getBoneList().size();

    size_t numberOfSpheres = numInstances * numberOfBones;
    size_t trsMatrixSize = numInstances * numberOfBones * sizeof(glm::mat4);

    mSphereWorldPosMatrices.clear();
    mSphereWorldPosMatrices.resize(numInstances);

    mSpherePerInstanceAnimData.clear();
    mSpherePerInstanceAnimData.resize(numInstances);

    for (size_t i = 0; i < instanceIds.size(); ++i) {
      InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getInstanceSettings();

      PerInstanceAnimData animData{};
      animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
      animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
      animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
      animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
      animData.blendFactor = instSettings.isAnimBlendFactor;

      mSpherePerInstanceAnimData.at(i) = animData;

      mSphereWorldPosMatrices.at(i) = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getWorldTransformMatrix();
    }

    /* we need to update descriptors after the upload if buffer size changed */
    bool doComputeDescriptorUpdates = false;
    if (mSphereModelRootMatrixBuffer.bufferSize != numInstances * sizeof(glm::mat4) ||
        mSpherePerInstanceAnimDataBuffer.bufferSize != numInstances * sizeof(PerInstanceAnimData) ||
        mSphereTRSMatrixBuffer.bufferSize != trsMatrixSize ||
        mSphereBoneMatrixBuffer.bufferSize != trsMatrixSize) {
      doComputeDescriptorUpdates = true;
    }

    mUploadToUBOTimer.start();
    ShaderStorageBuffer::uploadData(mRenderData, mSpherePerInstanceAnimDataBuffer, mSpherePerInstanceAnimData);
    ShaderStorageBuffer::uploadData(mRenderData, mSphereModelRootMatrixBuffer, mSphereWorldPosMatrices);
    mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

    /* resize SSBO if needed */
    ShaderStorageBuffer::checkForResize(mRenderData, mSphereBoneMatrixBuffer, trsMatrixSize);
    ShaderStorageBuffer::checkForResize(mRenderData, mSphereTRSMatrixBuffer, trsMatrixSize);

    if (doComputeDescriptorUpdates) {
      updateSphereComputeDescriptorSets();
    }

    /* in case data was changed */
    model->updateBoundingSphereAdjustments(mRenderData);

    /* record compute commands */
    VkResult result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: compute fence reset failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    if (!CommandBuffer::reset(mRenderData.rdComputeCommandBuffer, 0)) {
      Logger::log(1, "%s error: failed to reset compute command buffer\n", __FUNCTION__);
      return false;
    }

    if (!CommandBuffer::beginSingleShot(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to begin compute command buffer\n", __FUNCTION__);
      return false;
    }

    runBoundingSphereComputeShaders(model, numInstances, sphereModelOffset);
    sphereModelOffset += numberOfSpheres;
    mCollidingSphereCount += numberOfSpheres;

    if (!CommandBuffer::end(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to end compute command buffer\n", __FUNCTION__);
      return false;
    }

    /* submit compute commands */
    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &mRenderData.rdComputeCommandBuffer;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };

    /* we must wait for the compute shaders to finish before we can read the bone data */
    result = vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: waiting for compute fence failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  if (mCollidingSphereCount > 0) {
    mUploadToVBOTimer.start();
    VertexBuffer::uploadData(mRenderData, mSphereVertexBuffer, mCollidingSphereMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();
  }

  return true;
}

bool VkRenderer::createAllBoundingSpheres() {
  /* count total number of spheres to calculate */
  int totalSpheres = 0;
  for (const auto& model : mModelInstCamData.micModelList) {
    if (!model->hasAnimations()) {
      continue;
    }
    std::string modelName = model->getModelFileName();
    std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[modelName];

    size_t numberOfBones = model->getBoneList().size();
    size_t numInstances = instances.size();

    size_t numberOfSpheres = numInstances * numberOfBones;

    totalSpheres += numberOfSpheres;
  }

  bool doSphereDescriptorUpdates = false;
  if (mBoundingSphereBuffer.bufferSize != totalSpheres * sizeof(glm::vec4)) {
    doSphereDescriptorUpdates = true;
  }

  /* resize SSBO if needed */
  ShaderStorageBuffer::checkForResize(mRenderData, mBoundingSphereBuffer, totalSpheres * sizeof(glm::vec4));

  if (doSphereDescriptorUpdates) {
    updateSphereComputeDescriptorSets();
  }

  int sphereModelOffset = 0;
  for (const auto& model : mModelInstCamData.micModelList) {
    if (!model->hasAnimations()) {
      continue;
    }
    std::string modelName = model->getModelFileName();
    std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[modelName];

    size_t numberOfBones = model->getBoneList().size();
    size_t numInstances = instances.size();

    size_t numberOfSpheres = numInstances * numberOfBones;
    size_t trsMatrixSize = numInstances * numberOfBones * sizeof(glm::mat4);

    mSphereWorldPosMatrices.clear();
    mSphereWorldPosMatrices.resize(numInstances);

    mSpherePerInstanceAnimData.clear();
    mSpherePerInstanceAnimData.resize(numInstances);

    for (size_t i = 0; i < numInstances; ++i) {
      InstanceSettings instSettings = instances.at(i)->getInstanceSettings();

      PerInstanceAnimData animData{};
      animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
      animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
      animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
      animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
      animData.blendFactor = instSettings.isAnimBlendFactor;

      mSpherePerInstanceAnimData.at(i) = animData;

      mSphereWorldPosMatrices.at(i) = instances.at(i)->getWorldTransformMatrix();
    }

    /* we need to update descriptors after the upload if buffer size changed */
    bool doComputeDescriptorUpdates = false;
    if (mSphereModelRootMatrixBuffer.bufferSize != numInstances * sizeof(glm::mat4) ||
        mSpherePerInstanceAnimDataBuffer.bufferSize != numInstances * sizeof(PerInstanceAnimData) ||
        mSphereTRSMatrixBuffer.bufferSize != trsMatrixSize ||
        mSphereBoneMatrixBuffer.bufferSize != trsMatrixSize) {
      doComputeDescriptorUpdates = true;
    }

    mUploadToUBOTimer.start();
    ShaderStorageBuffer::uploadData(mRenderData, mSpherePerInstanceAnimDataBuffer, mSpherePerInstanceAnimData);
    ShaderStorageBuffer::uploadData(mRenderData, mSphereModelRootMatrixBuffer, mSphereWorldPosMatrices);
    mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

    /* resize SSBO if needed */
    ShaderStorageBuffer::checkForResize(mRenderData, mSphereBoneMatrixBuffer, trsMatrixSize);
    ShaderStorageBuffer::checkForResize(mRenderData, mSphereTRSMatrixBuffer, trsMatrixSize);

    if (doComputeDescriptorUpdates) {
      updateSphereComputeDescriptorSets();
    }

    /* in case data was changed */
    model->updateBoundingSphereAdjustments(mRenderData);

    /* record compute commands */
    VkResult result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: compute fence reset failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }

    if (!CommandBuffer::reset(mRenderData.rdComputeCommandBuffer, 0)) {
      Logger::log(1, "%s error: failed to reset compute command buffer\n", __FUNCTION__);
      return false;
    }

    if (!CommandBuffer::beginSingleShot(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to begin compute command buffer\n", __FUNCTION__);
      return false;
    }

    runBoundingSphereComputeShaders(model, numInstances, sphereModelOffset);
    sphereModelOffset += numberOfSpheres;
    mCollidingSphereCount += numberOfSpheres;

    if (!CommandBuffer::end(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to end compute command buffer\n", __FUNCTION__);
      return false;
    }

    /* submit compute commands */
    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &mRenderData.rdComputeCommandBuffer;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };

    /* we must wait for the compute shaders to finish before we can read the bone data */
    result = vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: waiting for compute fence failed (error: %i)\n", __FUNCTION__, result);
      return false;
    }
  }

  if (mCollidingSphereCount > 0) {
    mUploadToVBOTimer.start();
    VertexBuffer::uploadData(mRenderData, mSphereVertexBuffer, mSphereMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();
  }

  return true;
}

bool VkRenderer::draw(float deltaTime) {
  if (!mApplicationRunning) {
    return false;
  }

  /* no update on zero diff */
  if (deltaTime == 0.0f) {
    return true;
  }

  mRenderData.rdFrameTime = mFrameTimer.stop();
  mFrameTimer.start();

  /* reset timers and other values */
  mRenderData.rdMatricesSize = 0;
  mRenderData.rdUploadToUBOTime = 0.0f;
  mRenderData.rdUploadToVBOTime = 0.0f;
  mRenderData.rdMatrixGenerateTime = 0.0f;
  mRenderData.rdUIGenerateTime = 0.0f;
  mRenderData.rdUIDrawTime = 0.0f;
  mRenderData.rdNumberOfCollisions = 0;
  mRenderData.rdCollisionDebugDrawTime = 0.0f;
  mRenderData.rdCollisionCheckTime = 0.0f;
  mRenderData.rdBehaviorTime = 0.0f;
  mRenderData.rdInteractionTime = 0.0f;
  mRenderData.rdNumberOfInteractionCandidates = 0;
  mRenderData.rdInteractWithInstanceId = 0;
  mRenderData.rdFaceAnimTime = 0.0f;

  /* wait for both fences before getting the new framebuffer image */
  std::vector<VkFence> waitFences = { mRenderData.rdComputeFence, mRenderData.rdRenderFence };
  VkResult result = vkWaitForFences(mRenderData.rdVkbDevice.device,
    static_cast<uint32_t>(waitFences.size()), waitFences.data(), VK_TRUE, UINT64_MAX);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: waiting for fences failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  uint32_t imageIndex = 0;
  result = vkAcquireNextImageKHR(mRenderData.rdVkbDevice.device,
    mRenderData.rdVkbSwapchain.swapchain,
    UINT64_MAX,
    mRenderData.rdPresentSemaphore,
    VK_NULL_HANDLE,
    &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return recreateSwapchain();
  } else {
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
      Logger::log(1, "%s error: failed to acquire swapchain image. Error is '%i'\n", __FUNCTION__, result);
      return false;
    }
  }

  /* calculate the size of the lookup matrix buffer over all animated instances */
  size_t boneMatrixBufferSize = 0;
  size_t lookupBufferSize = 0;
  for (const auto& model : mModelInstCamData.micModelList) {
    size_t numberOfInstances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() && model->getBoneList().size() > 0) {
        size_t numberOfBones = model->getBoneList().size();

        boneMatrixBufferSize += numberOfBones * numberOfInstances;
        lookupBufferSize += numberOfInstances;
      }
    }
  }

  /* clear and resize world pos matrices */
  mWorldPosMatrices.clear();
  mWorldPosMatrices.resize(mModelInstCamData.micAssimpInstances.size());
  mPerInstanceAnimData.clear();
  mPerInstanceAnimData.resize(lookupBufferSize);
  mSelectedInstance.clear();
  mSelectedInstance.resize(mModelInstCamData.micAssimpInstances.size());
  mFaceAnimPerInstanceData.clear();
  mFaceAnimPerInstanceData.resize(mModelInstCamData.micAssimpInstances.size());

  /* save the selected instance for color highlight */
  std::shared_ptr<AssimpInstance> currentSelectedInstance = nullptr;
  if (mRenderData.rdApplicationMode == appMode::edit) {
    if (mRenderData.rdHighlightSelectedInstance) {
      currentSelectedInstance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);
      mRenderData.rdSelectedInstanceHighlightValue += deltaTime * 4.0f;
      if (mRenderData.rdSelectedInstanceHighlightValue > 2.0f) {
      mRenderData.rdSelectedInstanceHighlightValue = 0.1f;
      }
    }
  }

  /* get the bone matrix of the selected bone from the SSBO */
  std::shared_ptr<Camera> cam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
  CameraSettings camSettings = cam->getCameraSettings();

  int firstPersonCamWorldPos = -1;
  int firstPersonCamBoneMatrixPos = -1;

  /* we need to track the presence of animated models too */
  bool animatedModelLoaded = false;

  size_t instanceToStore = 0;
  size_t animatedInstancesToStore = 0;
  size_t animatedInstancesLookupToStore = 0;

  mQuadtree->clear();

  for (const auto& model : mModelInstCamData.micModelList) {
    size_t numberOfInstances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() && model->getBoneList().size() > 0) {
        size_t numberOfBones = model->getBoneList().size();
        ModelSettings modSettings = model->getModelSettings();

        animatedModelLoaded = true;

        mMatrixGenerateTimer.start();

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()];
        for (unsigned int i = 0; i < numberOfInstances; ++i) {
          instances.at(i)->updateInstanceSpeed(deltaTime);
          instances.at(i)->updateInstancePosition(deltaTime);
          instances.at(i)->updateAnimation(deltaTime);

          mWorldPosMatrices.at(instanceToStore + i) = instances.at(i)->getWorldTransformMatrix();

          InstanceSettings instSettings = instances.at(i)->getInstanceSettings();

          PerInstanceAnimData animData{};
          animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
          animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
          animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
          animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
          animData.blendFactor = instSettings.isAnimBlendFactor;

          if (model->hasHeadMovementAnimationsMapped()) {
            if (instSettings.isHeadLeftRightMove > 0.0f) {
              animData.headLeftRightAnimClipNum = modSettings.msHeadMoveClipMappings[headMoveDirection::left];
            } else {
              animData.headLeftRightAnimClipNum = modSettings.msHeadMoveClipMappings[headMoveDirection::right];
            }
            if (instSettings.isHeadUpDownMove > 0.0f) {
              animData.headUpDownAnimClipNum = modSettings.msHeadMoveClipMappings[headMoveDirection::up];
            } else {
              animData.headUpDownAnimClipNum = modSettings.msHeadMoveClipMappings[headMoveDirection::down];
            }
            animData.headLeftRightReplayTimestamp = std::fabs(instSettings.isHeadLeftRightMove) * model->getMaxClipDuration();
            animData.headUpDownReplayTimestamp = std::fabs(instSettings.isHeadUpDownMove) * model->getMaxClipDuration();
          }

          mPerInstanceAnimData.at(animatedInstancesLookupToStore + i) = animData;

          if (mRenderData.rdApplicationMode == appMode::edit) {
            if (currentSelectedInstance == instances.at(i)) {
              mSelectedInstance.at(instanceToStore + i).x = mRenderData.rdSelectedInstanceHighlightValue;
            } else {
              mSelectedInstance.at(instanceToStore + i).x = 1.0f;
            }

            if (mMousePick) {
              mSelectedInstance.at(instanceToStore + i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
            }
          } else {
            mSelectedInstance.at(instanceToStore + i).x = 1.0f;
          }

          if (camSettings.csCamType == cameraType::firstPerson && cam->getInstanceToFollow() &&
            instSettings.isInstanceIndexPosition == cam->getInstanceToFollow()->getInstanceSettings().isInstanceIndexPosition) {
            firstPersonCamWorldPos = instanceToStore + i;
            firstPersonCamBoneMatrixPos = animatedInstancesToStore + i * numberOfBones;
          }

          /* get AABB and calculate 2D boundaries */
          AABB instanceAABB = model->getAABB(instSettings);

          glm::vec2 position = glm::vec2(instanceAABB.getMinPos().x, instanceAABB.getMinPos().z);
          glm::vec2 size = glm::vec2(std::fabs(instanceAABB.getMaxPos().x - instanceAABB.getMinPos().x),
                                     std::fabs(instanceAABB.getMaxPos().z - instanceAABB.getMinPos().z));

          BoundingBox2D box{position, size};
          instances.at(i)->setBoundingBox(box);

          /* add instance to quadtree */
          mQuadtree->add(instSettings.isInstanceIndexPosition);

          mFaceAnimTimer.start();
          glm::vec4 morphData = glm::vec4(0.0f);
          if (instSettings.isFaceAnim != faceAnimation::none)  {
            morphData.x = instSettings.isFaceAnimWeight;
            morphData.y = static_cast<int>(instSettings.isFaceAnim) - 1;
            morphData.z = model->getAnimMeshVertexSize();
          }
          mFaceAnimPerInstanceData.at(animatedInstancesLookupToStore + i) = morphData;
          mRenderData.rdFaceAnimTime += mFaceAnimTimer.stop();
        }

        size_t trsMatrixSize = numberOfBones * numberOfInstances * sizeof(glm::mat4);

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
        mRenderData.rdMatricesSize += trsMatrixSize;

        instanceToStore += numberOfInstances;
        animatedInstancesToStore += numberOfInstances * numberOfBones;
        animatedInstancesLookupToStore += numberOfInstances;
      } else {
        /* non-animated models */
        mMatrixGenerateTimer.start();

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()];
        for (unsigned int i = 0; i < numberOfInstances; ++i) {
          mWorldPosMatrices.at(instanceToStore + i) = instances.at(i)->getWorldTransformMatrix();

          InstanceSettings instSettings = instances.at(i)->getInstanceSettings();

          if (mRenderData.rdApplicationMode == appMode::edit) {
            if (currentSelectedInstance == instances.at(i)) {
              mSelectedInstance.at(instanceToStore + i).x = mRenderData.rdSelectedInstanceHighlightValue;
            } else {
              mSelectedInstance.at(instanceToStore + i).x = 1.0f;
            }

            if (mMousePick) {
              mSelectedInstance.at(instanceToStore + i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
            }
          } else {
            mSelectedInstance.at(instanceToStore + i).x = 1.0f;
          }

          /* get AABB and calculate 2D boundaries */
          AABB instanceAABB = model->getAABB(instSettings);

          glm::vec2 position = glm::vec2(instanceAABB.getMinPos().x, instanceAABB.getMinPos().z);
          glm::vec2 size = glm::vec2(std::fabs(instanceAABB.getMaxPos().x - instanceAABB.getMinPos().x),
                                     std::fabs(instanceAABB.getMaxPos().z - instanceAABB.getMinPos().z));

          BoundingBox2D box{position, size};
          instances.at(i)->setBoundingBox(box);

          /* add instance to quadtree */
          mQuadtree->add(instSettings.isInstanceIndexPosition);
        }

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
        mRenderData.rdMatricesSize += numberOfInstances * sizeof(glm::mat4);

        instanceToStore += numberOfInstances;
      }
    }
  }

  /* we need to update descriptors after the upload if buffer size changed */
  bool doComputeDescriptorUpdates = false;
  if (mPerInstanceAnimDataBuffer.bufferSize != lookupBufferSize * sizeof(PerInstanceAnimData) ||
      mShaderTRSMatrixBuffer.bufferSize != boneMatrixBufferSize * sizeof(glm::mat4) ||
      mShaderBoneMatrixBuffer.bufferSize != boneMatrixBufferSize * sizeof(glm::mat4) ||
      mSelectedInstanceBuffer.bufferSize != lookupBufferSize * sizeof(glm::vec2) ||
      mFaceAnimPerInstanceDataBuffer.bufferSize != lookupBufferSize * sizeof(glm::vec4)) {
    doComputeDescriptorUpdates = true;
  }

  mUploadToUBOTimer.start();
  ShaderStorageBuffer::uploadData(mRenderData, mPerInstanceAnimDataBuffer, mPerInstanceAnimData);
  ShaderStorageBuffer::uploadData(mRenderData, mSelectedInstanceBuffer, mSelectedInstance);
  ShaderStorageBuffer::uploadData(mRenderData, mFaceAnimPerInstanceDataBuffer, mFaceAnimPerInstanceData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  /* resize SSBO if needed */
  ShaderStorageBuffer::checkForResize(mRenderData, mShaderTRSMatrixBuffer, boneMatrixBufferSize * sizeof(glm::mat4));
  ShaderStorageBuffer::checkForResize(mRenderData, mShaderBoneMatrixBuffer, boneMatrixBufferSize * sizeof(glm::mat4));

  if (doComputeDescriptorUpdates) {
    updateComputeDescriptorSets();
  }

  /* record compute commands */
  result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: compute fence reset failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  if (animatedModelLoaded) {
    uint32_t computeShaderModelOffset = 0;
    uint32_t computeShaderInstanceOffset = 0;
    if (!CommandBuffer::reset(mRenderData.rdComputeCommandBuffer, 0)) {
      Logger::log(1, "%s error: failed to reset compute command buffer\n", __FUNCTION__);
      return false;
    }

    if (!CommandBuffer::beginSingleShot(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to begin compute command buffer\n", __FUNCTION__);
      return false;
    }

    for (const auto& model : mModelInstCamData.micModelList) {
      size_t numberOfInstances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].size();
      if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

        /* compute shader for animated models only */
        if (model->hasAnimations() && model->getBoneList().size() > 0) {
          size_t numberOfBones = model->getBoneList().size();

          runComputeShaders(model, numberOfInstances, computeShaderModelOffset, computeShaderInstanceOffset);

          computeShaderModelOffset += numberOfInstances * numberOfBones;
          computeShaderInstanceOffset += numberOfInstances;
        }
      }
    }

    if (!CommandBuffer::end(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to end compute command buffer\n", __FUNCTION__);
      return false;
    }

    /* submit compute commands */
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.commandBufferCount = 1;
    computeSubmitInfo.pCommandBuffers = &mRenderData.rdComputeCommandBuffer;
    computeSubmitInfo.waitSemaphoreCount = 1;
    computeSubmitInfo.pWaitSemaphores = &mRenderData.rdGraphicSemaphore;
    computeSubmitInfo.pWaitDstStageMask = &waitStage;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };
  } else {
    /* do an empty submit if we don't have animated models to satisfy fence and semaphore */
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.waitSemaphoreCount = 1;
    computeSubmitInfo.pWaitSemaphores = &mRenderData.rdGraphicSemaphore;
    computeSubmitInfo.pWaitDstStageMask = &waitStage;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };
  }

  /* we must wait for the compute shaders to finish before we can read the bone data */
  result = vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence, VK_TRUE, UINT64_MAX);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: waiting for compute fence failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  /* extract bone matrix for first person view */
  if (camSettings.csCamType == cameraType::firstPerson && cam->getInstanceToFollow()) {
    std::shared_ptr<AssimpModel> model = cam->getInstanceToFollow()->getModel();
    size_t numberOfBones = model->getBoneList().size();
    if (numberOfBones > 0) {
      int selectedBone = camSettings.csFirstPersonBoneToFollow;

      glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), camSettings.csFirstPersonOffsets);

      /* TODO: save the index of the selected instance in both buffers */
      glm::mat4 boneMatrix = ShaderStorageBuffer::getSsboDataMat4(mRenderData, mShaderBoneMatrixBuffer,
        firstPersonCamBoneMatrixPos + selectedBone);
      cam->setBoneMatrix(mWorldPosMatrices.at(firstPersonCamWorldPos) * boneMatrix * offsetMatrix *
        glm::inverse(model->getBoneList().at(selectedBone)->getOffsetMatrix()));

      cam->setCameraSettings(camSettings);
    }
  }

  /* find interactions */
  mInteractionTimer.start();
  findInteractionInstances();
  mRenderData.rdInteractionTime += mInteractionTimer.stop();

  /* do collision checks after instances were updated and before drawing */
  mCollisionCheckTimer.start();
  checkForInstanceCollisions();
  checkForBorderCollisions();
  mRenderData.rdCollisionCheckTime += mCollisionCheckTimer.stop();

  handleMovementKeys();

  /* save mouse wheel (FOV/ortho scale) after 250ms of inactiviy */
  if (mMouseWheelScrolling) {
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
    float scrollDelta = std::chrono::duration_cast<std::chrono::microseconds>(now - mMouseWheelLastScrollTime).count() / 1'000'000.0f;
    if (scrollDelta > 0.25f) {
      mModelInstCamData.micSettingsContainer->applyEditCameraSettings(cam, camSettings, mSavedCameraWheelSettings);

      setConfigDirtyFlag(true);

      mMouseWheelScrolling = false;
    }
  }

  /* here it is safe to delete the Vulkan objects in the pending deletion models */
  if (mModelInstCamData.micDoDeletePendingAssimpModels) {
    mModelInstCamData.micDoDeletePendingAssimpModels = false;
    for (auto& model : mModelInstCamData.micPendingDeleteAssimpModels) {
      model->cleanup(mRenderData);
    }
    mModelInstCamData.micPendingDeleteAssimpModels.clear();
  }
  mModelInstCamData.micPendingDeleteAssimpModels.clear();

  mMatrixGenerateTimer.start();
  cam->updateCamera(mRenderData, deltaTime);

  if (camSettings.csCamProjection == cameraProjection::perspective) {
    mMatrices.projectionMatrix = glm::perspective(
      glm::radians(static_cast<float>(camSettings.csFieldOfView)),
        static_cast<float>(mRenderData.rdWidth) / static_cast<float>(mRenderData.rdHeight),
        0.1f, 500.0f);
  } else {
    float orthoScaling = camSettings.csOrthoScale;
    float aspect = static_cast<float>(mRenderData.rdWidth) / static_cast<float>(mRenderData.rdHeight) * orthoScaling;
    float leftRight = 1.0f * orthoScaling;
    float nearFar = 75.0f * orthoScaling;
    mMatrices.projectionMatrix = glm::ortho(-aspect, aspect, -leftRight, leftRight, -nearFar, nearFar);
  }

  mMatrices.viewMatrix = cam->getViewMatrix();

  mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();

  /* we need to update descriptors after the upload if buffer size changed */
  bool doDescriptorUpdates = false;
  if (mShaderModelRootMatrixBuffer.bufferSize != mWorldPosMatrices.size() * sizeof(glm::mat4) ||
    mShaderBoneMatrixBuffer.bufferSize != boneMatrixBufferSize * sizeof(glm::mat4)) {
    doDescriptorUpdates = true;
  }

  mUploadToUBOTimer.start();
  UniformBuffer::uploadData(mRenderData, mPerspectiveViewMatrixUBO, mMatrices);
  ShaderStorageBuffer::uploadData(mRenderData, mShaderModelRootMatrixBuffer, mWorldPosMatrices);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  if (doDescriptorUpdates) {
    updateDescriptorSets();
  }

  /* start with graphics rendering */
  result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdRenderFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error:  fence reset failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  if (!CommandBuffer::reset(mRenderData.rdCommandBuffer, 0)) {
    Logger::log(1, "%s error: failed to reset command buffer\n", __FUNCTION__);
    return false;
  }

  if (!CommandBuffer::beginSingleShot(mRenderData.rdCommandBuffer)) {
    Logger::log(1, "%s error: failed to begin command buffer\n", __FUNCTION__);
    return false;
  }

  std::vector<VkClearValue> colorClearValues;
  VkClearValue colorClearValue;
  colorClearValue.color = { { 0.25f, 0.25f, 0.25f, 1.0f } };
  colorClearValues.emplace_back(colorClearValue);
  if (mMousePick) {
    VkClearValue selectionClearValue;
    selectionClearValue.color = { { -1.0f } };
    colorClearValues.emplace_back(selectionClearValue);
  }

  VkClearValue depthValue;
  depthValue.depthStencil.depth = 1.0f;

  std::vector<VkClearValue> clearValues;
  clearValues.insert(clearValues.end(), colorClearValues.begin(), colorClearValues.end());
  clearValues.emplace_back(depthValue);

  VkRenderPassBeginInfo rpInfo{};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  if (mMousePick) {
    rpInfo.renderPass = mRenderData.rdSelectionRenderpass;
    rpInfo.framebuffer = mRenderData.rdSelectionFramebuffers.at(imageIndex);
  } else {
    rpInfo.renderPass = mRenderData.rdRenderpass;
    rpInfo.framebuffer = mRenderData.rdFramebuffers.at(imageIndex);
  }

  rpInfo.renderArea.offset.x = 0;
  rpInfo.renderArea.offset.y = 0;
  rpInfo.renderArea.extent = mRenderData.rdVkbSwapchain.extent;

  rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  rpInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(mRenderData.rdCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  /* flip viewport to be compatible with OpenGL */
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = static_cast<float>(mRenderData.rdVkbSwapchain.extent.height);
  viewport.width = static_cast<float>(mRenderData.rdVkbSwapchain.extent.width);
  viewport.height = -static_cast<float>(mRenderData.rdVkbSwapchain.extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };
  scissor.extent = mRenderData.rdVkbSwapchain.extent;

  vkCmdSetViewport(mRenderData.rdCommandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mRenderData.rdCommandBuffer, 0, 1, &scissor);

  int worldPosOffset = 0;
  int skinMatOffset = 0;
  for (const auto& model : mModelInstCamData.micModelList) {
    size_t numberOfInstances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() && model->getBoneList().size() > 0) {
        size_t numberOfBones = model->getBoneList().size();

        /* draw all meshes without morph anims first */
        if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
          vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpSkinningSelectionPipeline);

          vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpSkinningSelectionPipelineLayout, 1, 1,
           &mRenderData.rdAssimpSkinningSelectionDescriptorSet, 0, nullptr);
        } else {
          vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpSkinningPipeline);

          vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpSkinningPipelineLayout, 1, 1,
            &mRenderData.rdAssimpSkinningDescriptorSet, 0, nullptr);
        }

        mUploadToUBOTimer.start();
        mModelData.pkModelStride = numberOfBones;
        mModelData.pkWorldPosOffset = worldPosOffset;
        mModelData.pkSkinMatOffset = skinMatOffset;
        if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
          vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSkinningSelectionPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
        } else {
          vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSkinningPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
        }
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        model->drawInstancedNoMorphAnims(mRenderData, numberOfInstances, mMousePick);

        /* and if the model has morph anims, draw them in a separate pass  */
        if (model->hasAnimMeshes()) {
          if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
            vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
              mRenderData.rdAssimpSkinningMorphSelectionPipeline);

            vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
              mRenderData.rdAssimpSkinningMorphSelectionPipelineLayout, 1, 1,
              &mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet, 0, nullptr);
          } else {
            vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
              mRenderData.rdAssimpSkinningMorphPipeline);

            vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
              mRenderData.rdAssimpSkinningMorphPipelineLayout, 1, 1,
              &mRenderData.rdAssimpSkinningMorphDescriptorSet, 0, nullptr);
          }

          mUploadToUBOTimer.start();
          mModelData.pkModelStride = numberOfBones;
          mModelData.pkWorldPosOffset = worldPosOffset;
          mModelData.pkSkinMatOffset = skinMatOffset;
          if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
            vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSkinningMorphSelectionPipelineLayout,
              VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
          } else {
            vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSkinningMorphPipelineLayout,
              VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
          }
          mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

          model->drawInstancedMorphAnims(mRenderData, numberOfInstances, mMousePick);
        }

        worldPosOffset += numberOfInstances;
        skinMatOffset += numberOfInstances * numberOfBones;
      } else {
        /* non-animated models */
        if (mMousePick) {
          vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdAssimpSelectionPipeline);

          vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpSelectionPipelineLayout, 1, 1, &mRenderData.rdAssimpSelectionDescriptorSet, 0, nullptr);
        } else {
          vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdAssimpPipeline);

          vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpPipelineLayout, 1, 1, &mRenderData.rdAssimpDescriptorSet, 0, nullptr);
        }

        mUploadToUBOTimer.start();
        mModelData.pkWorldPosOffset = worldPosOffset;
        if (mMousePick) {
          vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSelectionPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
        } else {
          vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
        }
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        model->drawInstanced(mRenderData, numberOfInstances, mMousePick);

        worldPosOffset += numberOfInstances;
      }
    }
  }

  vkCmdEndRenderPass(mRenderData.rdCommandBuffer);

  if (!CommandBuffer::end(mRenderData.rdCommandBuffer)) {
    Logger::log(1, "%s error: failed to end command buffer\n", __FUNCTION__);
    return false;
  }

  /* draw coordinate lines */
  if (!CommandBuffer::reset(mRenderData.rdLineCommandBuffer, 0)) {
    Logger::log(1, "%s error: failed to reset line drawing command buffer\n", __FUNCTION__);
    return false;
  }

  if (!CommandBuffer::beginSingleShot(mRenderData.rdLineCommandBuffer)) {
    Logger::log(1, "%s error: failed to begin line drawing command buffer\n", __FUNCTION__);
    return false;
  }

  rpInfo.renderPass = mRenderData.rdLineRenderpass;
  rpInfo.framebuffer = mRenderData.rdFramebuffers.at(imageIndex);

  vkCmdBeginRenderPass(mRenderData.rdLineCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdSetViewport(mRenderData.rdLineCommandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mRenderData.rdLineCommandBuffer, 0, 1, &scissor);

  mLineIndexCount = 0;
  mLineMesh->vertices.clear();
  if (mRenderData.rdApplicationMode == appMode::edit) {
    if (mModelInstCamData.micSelectedInstance > 0) {
      InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance)->getInstanceSettings();

      /* draw coordiante arrows at origin of selected instance */
      switch(mRenderData.rdInstanceEditMode) {
        case instanceEditMode::move:
          mCoordArrowsMesh = mCoordArrowsModel.getVertexData();
          break;
        case instanceEditMode::rotate:
          mCoordArrowsMesh = mRotationArrowsModel.getVertexData();
          break;
        case instanceEditMode::scale:
          mCoordArrowsMesh = mScaleArrowsModel.getVertexData();
          break;
      }

      mLineIndexCount += mCoordArrowsMesh.vertices.size();
      std::for_each(mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end(),
                    [=](auto &n) {
                      n.color /= 2.0f;
                      n.position = glm::quat(glm::radians(instSettings.isWorldRotation)) * n.position;
                      n.position += instSettings.isWorldPosition;
                    });
      mLineMesh->vertices.insert(mLineMesh->vertices.end(),
        mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end());
    }
  }

  /* debug for interaction */
  mInteractionTimer.start();
  drawInteractionDebug();
  mRenderData.rdInteractionTime += mInteractionTimer.stop();

  /* draw AABB lines and bounding sphere of selected instance */
  mCollisionDebugDrawTimer.start();
  drawCollisionDebug();

  if (mLineIndexCount > 0) {
    mUploadToVBOTimer.start();
    VertexBuffer::uploadData(mRenderData, mLineVertexBuffer, *mLineMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();

    vkCmdBindPipeline(mRenderData.rdLineCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdLinePipeline);

    vkCmdBindDescriptorSets(mRenderData.rdLineCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      mRenderData.rdLinePipelineLayout, 0, 1, &mRenderData.rdLineDescriptorSet, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(mRenderData.rdLineCommandBuffer, 0, 1, &mLineVertexBuffer.buffer, &offset);
    vkCmdSetLineWidth(mRenderData.rdLineCommandBuffer, 3.0f);
    vkCmdDraw(mRenderData.rdLineCommandBuffer, static_cast<uint32_t>(mLineMesh->vertices.size()), 1, 0, 0);
  }

  /* draw bounding spheres */
  mCollidingSphereCount = 0;
  uint32_t sphereVertexCount = 0;

  switch (mRenderData.rdDrawBoundingSpheres) {
    case collisionDebugDraw::none:
      break;
    case collisionDebugDraw::colliding:
      if (mModelInstCamData.micInstanceCollisions.size() > 0) {
        createCollidingBoundingSpheres();
        sphereVertexCount = mCollidingSphereMesh.vertices.size();
      }
      break;
    case collisionDebugDraw::selected:
      /* no bounding sphere collision will be done with this setting, so run the computer shaders just for the selected instance */
      createSelectedBoundingSpheres();
      sphereVertexCount = mSphereMesh.vertices.size();
      break;
    case collisionDebugDraw::all:
      createAllBoundingSpheres();
      sphereVertexCount = mSphereMesh.vertices.size();
      break;
  }

  /* draw colliding spheres */
  if (mCollidingSphereCount > 0) {
    vkCmdBindPipeline(mRenderData.rdLineCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdSpherePipeline);

    vkCmdBindDescriptorSets(mRenderData.rdLineCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      mRenderData.rdSpherePipelineLayout, 0, 1, &mRenderData.rdSphereDescriptorSet, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(mRenderData.rdLineCommandBuffer, 0, 1, &mSphereVertexBuffer.buffer, &offset);
    vkCmdSetLineWidth(mRenderData.rdLineCommandBuffer, 3.0f);
    vkCmdDraw(mRenderData.rdLineCommandBuffer, sphereVertexCount, mCollidingSphereCount, 0, 0);
  }
  mRenderData.rdCollisionDebugDrawTime += mCollisionDebugDrawTimer.stop();

  vkCmdEndRenderPass(mRenderData.rdLineCommandBuffer);

  if (!CommandBuffer::end(mRenderData.rdLineCommandBuffer)) {
    Logger::log(1, "%s error: failed to end line drawing command buffer\n", __FUNCTION__);
    return false;
  }

  /* behavior update */
  mBehviorTimer.start();
  mBehavior->update(deltaTime);
  mRenderData.rdBehaviorTime += mBehviorTimer.stop();


  /* imGui overlay */
  mUIGenerateTimer.start();
  mUserInterface.createFrame(mRenderData);

  if (mRenderData.rdApplicationMode == appMode::edit) {
    mUserInterface.hideMouse(mMouseLock);
    mUserInterface.createSettingsWindow(mRenderData, mModelInstCamData);
  }

  /* always draw the status bar */
  mUserInterface.createStatusBar(mRenderData, mModelInstCamData);
  mUserInterface.createPositionsWindow(mRenderData, mModelInstCamData);
  mRenderData.rdUIGenerateTime += mUIGenerateTimer.stop();

  /* only loaded data right now */
  if (mGraphEditor->getShowEditor()) {
    mGraphEditor->updateGraphNodes(deltaTime);
  }

  if (mRenderData.rdApplicationMode != appMode::view) {
    mGraphEditor->createNodeEditorWindow(mRenderData, mModelInstCamData);
  }

  /* use separate ImGui render pass (with VK_ATTACHMENT_LOAD_OP_LOAD) to avoid renderpass incomatibilities */
  if (!CommandBuffer::reset(mRenderData.rdImGuiCommandBuffer, 0)) {
    Logger::log(1, "%s error: failed to reset ImGui command buffer\n", __FUNCTION__);
    return false;
  }

  if (!CommandBuffer::beginSingleShot(mRenderData.rdImGuiCommandBuffer)) {
    Logger::log(1, "%s error: failed to begin ImGui command buffer\n", __FUNCTION__);
    return false;
  }

  rpInfo.renderPass = mRenderData.rdImGuiRenderpass;
  rpInfo.framebuffer = mRenderData.rdFramebuffers.at(imageIndex);

  vkCmdBeginRenderPass(mRenderData.rdImGuiCommandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdSetViewport(mRenderData.rdImGuiCommandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(mRenderData.rdImGuiCommandBuffer, 0, 1, &scissor);

  mUIDrawTimer.start();
  mUserInterface.render(mRenderData);
  mRenderData.rdUIDrawTime += mUIDrawTimer.stop();

  vkCmdEndRenderPass(mRenderData.rdImGuiCommandBuffer);

  if (!CommandBuffer::end(mRenderData.rdImGuiCommandBuffer)) {
    Logger::log(1, "%s error: failed to end ImGui command buffer\n", __FUNCTION__);
    return false;
  }

  /* submit command buffer */
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  //std::vector<VkSemaphore> waitSemaphores = { mRenderData.rdComputeSemaphore, m//RenderData.rdPresentSemaphore };
  //std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

  std::vector<VkSemaphore> waitSemaphores = { mRenderData.rdPresentSemaphore };
  std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
  /* compute shader: contine if in vertex input ready
   * vertex shader: wait for color attachment output ready */
  submitInfo.pWaitDstStageMask = waitStages.data();

  submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
  submitInfo.pWaitSemaphores = waitSemaphores.data();

  std::vector<VkSemaphore> signalSemaphores = { mRenderData.rdRenderSemaphore, mRenderData.rdGraphicSemaphore };

  submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
  submitInfo.pSignalSemaphores = signalSemaphores.data();

  std::vector<VkCommandBuffer> commandBuffers =
    { mRenderData.rdCommandBuffer, mRenderData.rdLineCommandBuffer, mRenderData.rdImGuiCommandBuffer };

  submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
  submitInfo.pCommandBuffers = commandBuffers.data();

  result = vkQueueSubmit(mRenderData.rdGraphicsQueue, 1, &submitInfo, mRenderData.rdRenderFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to submit draw command buffer (%i)\n", __FUNCTION__, result);
    return false;
  }

  /* we must wait for the image to be created before we can pick  */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    if (mMousePick) {
      /* wait for queue to be idle */
      vkQueueWaitIdle(mRenderData.rdGraphicsQueue);

      float selectedInstanceId = SelectionFramebuffer::getPixelValueFromPos(mRenderData, mMouseXPos, mMouseYPos);

      if (selectedInstanceId >= 0.0f) {
        mModelInstCamData.micSelectedInstance = static_cast<int>(selectedInstanceId);
      } else {
        mModelInstCamData.micSelectedInstance = 0;
      }
      mModelInstCamData.micSettingsContainer->applySelectInstance(mModelInstCamData.micSelectedInstance, mSavedSelectedInstanceId);
      mMousePick = false;
    }
  }

  /* trigger swapchain image presentation */
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &mRenderData.rdRenderSemaphore;

  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &mRenderData.rdVkbSwapchain.swapchain;

  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(mRenderData.rdPresentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    return recreateSwapchain();
  } else {
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to present swapchain image\n", __FUNCTION__);
      return false;
    }
  }

  return true;
}

void VkRenderer::cleanup() {
  VkResult result = vkDeviceWaitIdle(mRenderData.rdVkbDevice.device);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s fatal error: could not wait for device idle (error: %i)\n", __FUNCTION__, result);
    return;
  }

  /* delete models to destroy Vulkan objects */
  for (const auto& model : mModelInstCamData.micModelList) {
    model->cleanup(mRenderData);
  }

  mUserInterface.cleanup(mRenderData);

  SyncObjects::cleanup(mRenderData);
  CommandBuffer::cleanup(mRenderData, mRenderData.rdCommandPool, mRenderData.rdCommandBuffer);
  CommandBuffer::cleanup(mRenderData, mRenderData.rdCommandPool, mRenderData.rdImGuiCommandBuffer);
  CommandBuffer::cleanup(mRenderData, mRenderData.rdCommandPool, mRenderData.rdLineCommandBuffer);
  CommandBuffer::cleanup(mRenderData, mRenderData.rdComputeCommandPool, mRenderData.rdComputeCommandBuffer);
  CommandPool::cleanup(mRenderData, mRenderData.rdCommandPool);
  CommandPool::cleanup(mRenderData, mRenderData.rdComputeCommandPool);

  VertexBuffer::cleanup(mRenderData, mLineVertexBuffer);
  VertexBuffer::cleanup(mRenderData, mSphereVertexBuffer);

  Framebuffer::cleanup(mRenderData);
  SelectionFramebuffer::cleanup(mRenderData);

  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSkinningPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSelectionPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSkinningSelectionPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSkinningMorphPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSkinningMorphSelectionPipeline);
  LinePipeline::cleanup(mRenderData, mRenderData.rdLinePipeline);
  LinePipeline::cleanup(mRenderData, mRenderData.rdSpherePipeline);

  ComputePipeline::cleanup(mRenderData, mRenderData.rdAssimpComputeTransformPipeline);
  ComputePipeline::cleanup(mRenderData, mRenderData.rdAssimpComputeHeadMoveTransformPipeline);
  ComputePipeline::cleanup(mRenderData, mRenderData.rdAssimpComputeMatrixMultPipeline);
  ComputePipeline::cleanup(mRenderData, mRenderData.rdAssimpComputeBoundingSpheresPipeline);

  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSkinningPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpComputeTransformaPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpComputeMatrixMultPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpComputeBoundingSpheresPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSelectionPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSkinningSelectionPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSkinningMorphPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSkinningMorphSelectionPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdLinePipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdSpherePipelineLayout);

  Renderpass::cleanup(mRenderData, mRenderData.rdRenderpass);
  SecondaryRenderpass::cleanup(mRenderData, mRenderData.rdImGuiRenderpass);
  SecondaryRenderpass::cleanup(mRenderData, mRenderData.rdLineRenderpass);
  SelectionRenderpass::cleanup(mRenderData);

  UniformBuffer::cleanup(mRenderData, mPerspectiveViewMatrixUBO);
  ShaderStorageBuffer::cleanup(mRenderData, mShaderTRSMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mPerInstanceAnimDataBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mShaderModelRootMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mShaderBoneMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mSelectedInstanceBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mBoundingSphereBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mSphereModelRootMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mSpherePerInstanceAnimDataBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mSphereTRSMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mSphereBoneMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mFaceAnimPerInstanceDataBuffer);

  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpSkinningDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpComputeTransformDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpComputeMatrixMultDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpSelectionDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpSkinningSelectionDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpSkinningMorphDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpSkinningMorphSelectionDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdLineDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdSphereDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpComputeSphereTransformDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpComputeSphereMatrixMultDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1,
    &mRenderData.rdAssimpComputeBoundingSpheresDescriptorSet);

  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSkinningDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpTextureDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeTransformDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeTransformPerModelDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeMatrixMultDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeMatrixMultPerModelDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeBoundingSpheresDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeBoundingSpheresPerModelDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSelectionDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSkinningSelectionDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSkinningMorphDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSkinningMorphSelectionDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSkinningMorphPerModelDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdLineDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdSphereDescriptorLayout, nullptr);

  vkDestroyDescriptorPool(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, nullptr);

  vkDestroyImageView(mRenderData.rdVkbDevice.device, mRenderData.rdDepthImageView, nullptr);
  vmaDestroyImage(mRenderData.rdAllocator, mRenderData.rdDepthImage, mRenderData.rdDepthImageAlloc);

  vkDestroyImageView(mRenderData.rdVkbDevice.device, mRenderData.rdSelectionImageView, nullptr);
  vmaDestroyImage(mRenderData.rdAllocator, mRenderData.rdSelectionImage, mRenderData.rdSelectionImageAlloc);

  vmaDestroyAllocator(mRenderData.rdAllocator);

  mRenderData.rdVkbSwapchain.destroy_image_views(mRenderData.rdSwapchainImageViews);
  vkb::destroy_swapchain(mRenderData.rdVkbSwapchain);

  vkb::destroy_device(mRenderData.rdVkbDevice);
  vkb::destroy_surface(mRenderData.rdVkbInstance.instance, mSurface);
  vkb::destroy_instance(mRenderData.rdVkbInstance);

  Logger::log(1, "%s: Vulkan renderer destroyed\n", __FUNCTION__);
}
