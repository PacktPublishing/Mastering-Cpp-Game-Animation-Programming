#include <algorithm>

#include <imgui_impl_glfw.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <set>
#include <map>

#include "OGLRenderer.h"
#include "InstanceSettings.h"
#include "CameraSettings.h"
#include "ModelSettings.h"
#include "AssimpSettingsContainer.h"
#include "GraphEditor.h"
#include "Logger.h"

OGLRenderer::OGLRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;
}

bool OGLRenderer::init(unsigned int width, unsigned int height) {
  /* randomize rand() */
  std::srand(static_cast<int>(time(nullptr)));

  /* init app mode map first */
  mRenderData.mAppModeMap[appMode::edit] = "Edit";
  mRenderData.mAppModeMap[appMode::view] = "View";

  /* save orig window title, add current mode */
  mOrigWindowTitle = getWindowTitle();
  setModeInWindowTitle();

  /* required for perspective */
  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  /* initalize GLAD */
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    Logger::log(1, "%s error: failed to initialize GLAD\n", __FUNCTION__);
    return false;
  }

  if (!GLAD_GL_VERSION_4_6) {
    Logger::log(1, "%s error: failed to get at least OpenGL 4.6\n", __FUNCTION__);
    return false;
  }

  GLint majorVersion, minorVersion;
  glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
  glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
  Logger::log(1, "%s: OpenGL %d.%d initializeed\n", __FUNCTION__, majorVersion, minorVersion);

  if (!mFramebuffer.init(width, height)) {
    Logger::log(1, "%s error: could not init Framebuffer\n", __FUNCTION__);
    return false;
  }
  Logger::log(1, "%s: framebuffer succesfully initialized\n", __FUNCTION__);

  mLineVertexBuffer.init();
  Logger::log(1, "%s: line vertex buffer successfully created\n", __FUNCTION__);

  size_t uniformMatrixBufferSize = 3 * sizeof(glm::mat4);
  mUniformBuffer.init(uniformMatrixBufferSize);
  Logger::log(1, "%s: matrix uniform buffer (size %i bytes) successfully created\n", __FUNCTION__, uniformMatrixBufferSize);

  if (!mLineShader.loadShaders("shader/line.vert", "shader/line.frag")) {
    Logger::log(1, "%s: line shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mSphereShader.loadShaders("shader/sphere_instance.vert", "shader/sphere_instance.frag")) {
    Logger::log(1, "%s: sphere shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpShader.loadShaders("shader/assimp.vert", "shader/assimp.frag")) {
    Logger::log(1, "%s: Assimp shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSkinningShader.loadShaders("shader/assimp_skinning.vert", "shader/assimp_skinning.frag")) {
    Logger::log(1, "%s: Assimp GPU skinning shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpSkinningShader.getUniformLocation("aModelStride")) {
    Logger::log(1, "%s: could not find symobl 'aModelStride' in GPU skinning shader\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSkinningMorphShader.loadShaders("shader/assimp_skinning_morph.vert", "shader/assimp_skinning_morph.frag")) {
    Logger::log(1, "%s: Assimp GPU skinning with morph anims shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpSkinningMorphShader.getUniformLocation("aModelStride")) {
    Logger::log(1, "%s: could not find symobl 'aModelStride' in GPU skinning with morph anims shader\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSelectionShader.loadShaders("shader/assimp_selection.vert", "shader/assimp_selection.frag")) {
    Logger::log(1, "%s: Assimp slection shader loading failed\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSkinningSelectionShader.loadShaders("shader/assimp_skinning_selection.vert", "shader/assimp_skinning_selection.frag")) {
    Logger::log(1, "%s: Assimp GPU skinning selection shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpSkinningSelectionShader.getUniformLocation("aModelStride")) {
    Logger::log(1, "%s: could not find symobl 'aModelStride' in GPU skinning selection shader\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpSkinningMorphSelectionShader.loadShaders("shader/assimp_skinning_morph_selection.vert", "shader/assimp_skinning_morph_selection.frag")) {
    Logger::log(1, "%s: Assimp GPU skinning with morph anims and selection shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpSkinningMorphSelectionShader.getUniformLocation("aModelStride")) {
    Logger::log(1, "%s: could not find symobl 'aModelStride' in GPU skinning with morph anims and selection shader\n", __FUNCTION__);
    return false;
  }

  if (!mAssimpTransformComputeShader.loadComputeShader("shader/assimp_instance_transform.comp")) {
    Logger::log(1, "%s: Assimp GPU node transform compute shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpTransformHeadMoveComputeShader.loadComputeShader("shader/assimp_instance_headmove_transform.comp")) {
    Logger::log(1, "%s: Assimp GPU node transform with head move compute shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpMatrixComputeShader.loadComputeShader("shader/assimp_instance_matrix_mult.comp")) {
    Logger::log(1, "%s: Assimp GPU matrix compute shader loading failed\n", __FUNCTION__);
    return false;
  }
  if (!mAssimpBoundingBoxComputeShader.loadComputeShader("shader/assimp_instance_bounding_spheres.comp")) {
    Logger::log(1, "%s: Assimp GPU bounding spheres matrix compute shader loading failed\n", __FUNCTION__);
    return false;
  }

  Logger::log(1, "%s: shaders succesfully loaded\n", __FUNCTION__);

  /* add backface culling and depth test already here */
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glLineWidth(3.0);
  Logger::log(1, "%s: rendering defaults set\n", __FUNCTION__);

  /* init quadtree with some default values */
  mWorldBoundaries = std::make_shared<BoundingBox2D>(mRenderData.rdWorldStartPos, mRenderData.rdWorldSize);
  initQuadTree(10, 5);
  Logger::log(1, "%s: quadtree initialized\n", __FUNCTION__);

  mModelInstCamData.micQuadTreeFindAllIntersectionsCallbackFunction = [this]() { return mQuadtree->findAllIntersections(); };
  mModelInstCamData.micQuadTreeGetBoxesCallback = [this]() { return mQuadtree->getTreeBoxes(); };
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

  mRenderData.rdAppExitCallback = [this]() { doExitApplication(); };
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
  mLineMesh = std::make_shared<OGLLineMesh>();
  Logger::log(1, "%s: line mesh storage initialized\n", __FUNCTION__);

  mAABBMesh = std::make_shared<OGLLineMesh>();
  Logger::log(1, "%s: AABB line mesh storage initialized\n", __FUNCTION__);

  mSphereModel = SphereModel(1.0, 5, 8, glm::vec3(1.0f, 1.0f, 1.0f));
  mSphereMesh = mSphereModel.getVertexData();
  Logger::log(1, "%s: Sphere line mesh storage initialized\n", __FUNCTION__);

  mCollidingSphereModel = SphereModel(1.0, 5, 8, glm::vec3(1.0f, 0.0f, 0.0f));
  mCollidingSphereMesh = mCollidingSphereModel.getVertexData();
  Logger::log(1, "%s: Colliding sphere line mesh storage initialized\n", __FUNCTION__);

  mBehavior = std::make_shared<Behavior>();
  mInstanceNodeActionCallback = [this](int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
    updateInstanceSettings(instanceId, nodeType, updateType, data, extraSetting);
  };
  mBehavior->setNodeActionCallback(mInstanceNodeActionCallback);
  Logger::log(1, "%s: behavior data initialized\n", __FUNCTION__);

  mGraphEditor = std::make_shared<GraphEditor>();
  Logger::log(1, "%s: graph editor initialized\n", __FUNCTION__);

  /* try to load the default configuration file */
  if (loadConfigFile(mDefaultConfigFileName)) {
    Logger::log(1, "%s: loaded default config file '%s'\n", __FUNCTION__, mDefaultConfigFileName.c_str());
  } else {
    Logger::log(1, "%s: could not load default config file '%s'\n", __FUNCTION__, mDefaultConfigFileName.c_str());
    /* only add null instance if we don't have default config */
    createEmptyConfig();
  }

  mUserInterface.init(mRenderData);
  Logger::log(1, "%s: user interface initialized\n", __FUNCTION__);

  Logger::log(1, "%s: all done, starting application\n", __FUNCTION__);
  mFrameTimer.start();
  mApplicationRunning = true;

  return true;
}

bool OGLRenderer::loadConfigFile(std::string configFileName) {
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

    /* migration config version 4.0 to 5.0+  */
    if (yamlFileVersion == "4.0") {
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

bool OGLRenderer::saveConfigFile(std::string configFileName) {
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

void OGLRenderer::createEmptyConfig() {
  removeAllModelsAndInstances();
  loadDefaultFreeCam();
}

void OGLRenderer::requestExitApplication() {
  /* set app mode back to edit to show windows */
  mRenderData.rdApplicationMode = appMode::edit;
  mRenderData.rdRequestApplicationExit = true;
}

void OGLRenderer::doExitApplication() {
  mApplicationRunning = false;
}

void OGLRenderer::undoLastOperation() {
  if (mModelInstCamData.micSettingsContainer->getUndoSize() == 0) {
    return;
  }

  mModelInstCamData.micSettingsContainer->undo();
  /* we need to update the index numbers in case instances were deleted,
   * and the settings files still contain the old index number */
  enumerateInstances();

  int selectedInstace = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  if (selectedInstace < mModelInstCamData.micAssimpInstances.size()) {
    mModelInstCamData.micSelectedInstance = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  } else {
    mModelInstCamData.micSelectedInstance = 0;
  }

  /* if we made all changes undone, the config is no longer dirty */
  if (mModelInstCamData.micSettingsContainer->getUndoSize() == 0) {
    setConfigDirtyFlag(false);
  }
}

void OGLRenderer::redoLastOperation() {
  if (mModelInstCamData.micSettingsContainer->getRedoSize() == 0) {
    return;
  }

  mModelInstCamData.micSettingsContainer->redo();
  enumerateInstances();

  int selectedInstace = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  if (selectedInstace < mModelInstCamData.micAssimpInstances.size()) {
    mModelInstCamData.micSelectedInstance = mModelInstCamData.micSettingsContainer->getCurrentInstance();
  } else {
    mModelInstCamData.micSelectedInstance = 0;
  }

  /* if any changes have been re-done, the config is dirty */
  if (mModelInstCamData.micSettingsContainer->getUndoSize() > 0) {
    setConfigDirtyFlag(true);
  }
}

void OGLRenderer::addNullModelAndInstance(){
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

void OGLRenderer::createSettingsContainerCallbacks() {
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

void OGLRenderer::clearUndoRedoStacks() {
  mModelInstCamData.micSettingsContainer->removeStacks();
}

void OGLRenderer::removeAllModelsAndInstances() {
  mModelInstCamData.micSelectedInstance = 0;
  mModelInstCamData.micSelectedModel = 0;

  mModelInstCamData.micAssimpInstances.erase(mModelInstCamData.micAssimpInstances.begin(),mModelInstCamData.micAssimpInstances.end());
  mModelInstCamData.micAssimpInstancesPerModel.clear();
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

void OGLRenderer::resetCollisionData() {
  mModelInstCamData.micInstanceCollisions.clear();

  mRenderData.rdNumberOfCollisions = 0;
  mRenderData.rdCheckCollisions = collisionChecks::none;
  mRenderData.rdDrawCollisionAABBs = collisionDebugDraw::none;
  mRenderData.rdDrawBoundingSpheres = collisionDebugDraw::none;
}

void OGLRenderer::loadDefaultFreeCam() {
  mModelInstCamData.micCameras.clear();

  std::shared_ptr<Camera> freeCam = std::make_shared<Camera>();
  freeCam->setName("FreeCam");
  mModelInstCamData.micCameras.emplace_back(freeCam);

  mModelInstCamData.micSelectedCamera = 0;
}

bool OGLRenderer::hasModel(std::string modelFileName) {
  auto modelIter =  std::find_if(mModelInstCamData.micModelList.begin(), mModelInstCamData.micModelList.end(),
    [modelFileName](const auto& model) {
      return model->getModelFileNamePath() == modelFileName || model->getModelFileName() == modelFileName;
    });
  return modelIter != mModelInstCamData.micModelList.end();
}

std::shared_ptr<AssimpModel> OGLRenderer::getModel(std::string modelFileName) {
  auto modelIter =  std::find_if(mModelInstCamData.micModelList.begin(), mModelInstCamData.micModelList.end(),
    [modelFileName](const auto& model) {
      return model->getModelFileNamePath() == modelFileName || model->getModelFileName() == modelFileName;
    });
  if (modelIter != mModelInstCamData.micModelList.end()) {
    return *modelIter;
  }
  return nullptr;
}

bool OGLRenderer::addModel(std::string modelFileName, bool addInitialInstance, bool withUndo) {
  if (hasModel(modelFileName)) {
    Logger::log(1, "%s warning: model '%s' already existed, skipping\n", __FUNCTION__, modelFileName.c_str());
    return false;
  }

  std::shared_ptr<AssimpModel> model = std::make_shared<AssimpModel>();
  if (!model->loadModel(modelFileName)) {
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


  /* create AABBs for the model*/
  createAABBLookup(model);

  return true;
}

void OGLRenderer::addExistingModel(std::shared_ptr<AssimpModel> model, int indexPos) {
  Logger::log(2, "%s: inserting model %s on pos %i\n", __FUNCTION__, model->getModelFileName().c_str(), indexPos);
  mModelInstCamData.micModelList.insert(mModelInstCamData.micModelList.begin() + indexPos, model);
}

void OGLRenderer::deleteModel(std::string modelFileName, bool withUndo) {
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

  mModelInstCamData.micModelList.erase(
    std::remove_if(
      mModelInstCamData.micModelList.begin(),
      mModelInstCamData.micModelList.end(),
      [modelFileName](std::shared_ptr<AssimpModel> model) { return model->getModelFileName() == modelFileName; }
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

std::shared_ptr<AssimpInstance> OGLRenderer::getInstanceById(int instanceId) {
  if (instanceId < mModelInstCamData.micAssimpInstances.size()) {
    return mModelInstCamData.micAssimpInstances.at(instanceId);
  } else {
    Logger::log(1, "%s error: instance id %i out of range, we only have %i instances\n", __FUNCTION__, instanceId,  mModelInstCamData.micAssimpInstances.size());
    return mModelInstCamData.micAssimpInstances.at(0);
  }
}

std::shared_ptr<AssimpInstance> OGLRenderer::addInstance(std::shared_ptr<AssimpModel> model, bool withUndo) {
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

void OGLRenderer::addExistingInstance(std::shared_ptr<AssimpInstance> instance, int indexPos, int indexPerModelPos) {
  Logger::log(2, "%s: inserting instance on pos %i\n", __FUNCTION__, indexPos);
  mModelInstCamData.micAssimpInstances.insert(mModelInstCamData.micAssimpInstances.begin() + indexPos, instance);
  mModelInstCamData.micAssimpInstancesPerModel[instance->getModel()->getModelFileName()].insert(mModelInstCamData.micAssimpInstancesPerModel[instance->getModel()->getModelFileName()].begin() +
    indexPerModelPos, instance);

  enumerateInstances();
  updateTriangleCount();
}

void OGLRenderer::addInstances(std::shared_ptr<AssimpModel> model, int numInstances) {
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

void OGLRenderer::deleteInstance(std::shared_ptr<AssimpInstance> instance, bool withUndo) {
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

void OGLRenderer::cloneInstance(std::shared_ptr<AssimpInstance> instance) {
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
void OGLRenderer::cloneInstances(std::shared_ptr<AssimpInstance> instance, int numClones){
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

    enumerateInstances();

    /* add behavior tree after new id was set */
    InstanceSettings newInstanceSettings = newInstance->getInstanceSettings();
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

void OGLRenderer::centerInstance(std::shared_ptr<AssimpInstance> instance) {
  InstanceSettings instSettings = instance->getInstanceSettings();
  mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera)->moveCameraTo(instSettings.isWorldPosition + glm::vec3(5.0f));
}

std::vector<glm::vec2> OGLRenderer::get2DPositionOfAllInstances() {
  std::vector<glm::vec2> positions;

  /* skip null instance */
  for (int i = 1; i < mModelInstCamData.micAssimpInstances.size(); ++i) {
    glm::vec3 modelPos = mModelInstCamData.micAssimpInstances.at(i)->getWorldPosition();
    positions.emplace_back(modelPos.x, modelPos.z);
  }

  return positions;
}

void OGLRenderer::editGraph(std::string graphName) {
  if (mModelInstCamData.micBehaviorData.count(graphName) > 0) {
    mGraphEditor->loadData(mModelInstCamData.micBehaviorData.at(graphName)->getBehaviorData());
  } else {
    Logger::log(1, "%s error: graph '%s' not found\n", __FUNCTION__, graphName.c_str());
  }
}

std::shared_ptr<SingleInstanceBehavior> OGLRenderer::createEmptyGraph() {
  mGraphEditor->createEmptyGraph();
  return mGraphEditor->getData();
}

void OGLRenderer::initQuadTree(int thresholdPerBox, int maxDepth) {
  mQuadtree = std::make_shared<QuadTree>(mWorldBoundaries, thresholdPerBox, maxDepth);

  /* quadtree needs to get bounding box of the instances */
  mQuadtree->instanceGetBoundingBox2DCallback = [this](int instanceId) {
    return mModelInstCamData.micAssimpInstances.at(instanceId)->getBoundingBox();
  };
}

std::shared_ptr<BoundingBox2D> OGLRenderer::getWorldBoundaries() {
  return mWorldBoundaries;
}

void OGLRenderer::createAABBLookup(std::shared_ptr<AssimpModel> model) {
  const int LOOKUP_SIZE = 1023;
  /* we use a single instance per clip*/
  size_t numberOfClips = model->getAnimClips().size();

  mPerInstanceAnimData.resize(numberOfClips);

  auto boneList =  model->getBoneList();
  size_t numberOfBones = boneList.size();

  /* we need valid model with triangels and animations */
  if (numberOfClips > 0 && numberOfBones > 0 &&
      model->getTriangleCount() > 0) {

    Logger::log(1, "%s: playing animations for model %s\n", __FUNCTION__, model->getModelFileName().c_str());

    /* we MUST set the bone offsets to identity matrices to get the skeleton data */
    std::vector<glm::mat4> emptyBoneOfssets(numberOfBones, glm::mat4(1.0f));
    mEmptyBoneOffsetBuffer.uploadSsboData(emptyBoneOfssets);

    std::vector<std::vector<AABB>> aabbLookups;
    aabbLookups.resize(numberOfClips);

    std::vector<glm::mat4> boneMatrix;

    size_t numberOfBones = model->getBoneList().size();
    size_t trsMatrixSize = numberOfBones * numberOfClips * sizeof(glm::mat4);
    mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
    mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);
    mPerInstanceAnimDataBuffer.checkForResize(numberOfClips);

    /* some models have a scaling set here... */
    glm::mat4 rootTransformMat = glm::transpose(model->getRootTranformationMatrix());

    /* our axis aligned bounding box */
    AABB aabb;

    /* play all animation steps */
    float timeScaleFactor = model->getMaxClipDuration() / static_cast<float>(LOOKUP_SIZE);
    for (int lookups = 0; lookups < LOOKUP_SIZE; lookups++) {
      for (size_t i = 0; i < numberOfClips; ++i) {

        PerInstanceAnimData animData{};
        animData.firstAnimClipNum = i;
        animData.secondAnimClipNum = 0;
        animData.firstClipReplayTimestamp = lookups * timeScaleFactor;
        animData.secondClipReplayTimestamp = 0.0f;
        animData.blendFactor = 0.0f;

        mPerInstanceAnimData.at(i) = animData;
      }

      /* do a single iteration of all clips in parallel */
      mAssimpTransformComputeShader.use();

      mUploadToUBOTimer.start();
      model->bindAnimLookupBuffer(0);
      mPerInstanceAnimDataBuffer.uploadSsboData(mPerInstanceAnimData, 1);
      mShaderTRSMatrixBuffer.bind(2);
      mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

      glDispatchCompute(numberOfBones, std::ceil(numberOfClips/ 32.0f), 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


      mAssimpMatrixComputeShader.use();

      mUploadToUBOTimer.start();
      mShaderTRSMatrixBuffer.bind(0);
      model->bindBoneParentBuffer(1);
      mEmptyBoneOffsetBuffer.bind(2);
      mShaderBoneMatrixBuffer.bind(3);
      mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

      glDispatchCompute(numberOfBones, std::ceil(numberOfClips/ 32.0f), 1);
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      /* extract bone matrix from SSBO */
      boneMatrix = mShaderBoneMatrixBuffer.getSsboDataMat4();

      /* and loop over clips and bones */
      for (size_t i = 0; i < numberOfClips; ++i) {
        /* add first point */
        glm::vec3 bonePos = (rootTransformMat * boneMatrix.at(numberOfBones * i))[3];
        aabb.create(bonePos);

        /* extend AABB for other points */
        for (size_t j = 1; j < numberOfBones; ++j) {
          /* Shader:  uint index = node + numberOfBones * instance; */
          glm::vec3 bonePos = (rootTransformMat * boneMatrix.at(j + numberOfBones * i))[3];
          aabb.addPoint(bonePos);
        }

        aabbLookups.at(i).emplace_back(aabb);
      }
    }

    model->setAABBLokkup(aabbLookups);
  }
}

void OGLRenderer::addBehavior(int instanceId, std::shared_ptr<SingleInstanceBehavior> behavior) {
  if (mModelInstCamData.micAssimpInstances.size() < instanceId) {
    Logger::log(1, "%s error: number of instances is smaller than instance id %i\n", __FUNCTION__, instanceId);
    return;
  }

  //std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(instanceId);
  //instance->isNPC(true);

  mBehviorTimer.start();
  mBehavior->addInstance(instanceId, behavior);
  mRenderData.rdBehaviorTime += mBehviorTimer.stop();
  Logger::log(1, "%s: added behavior %s to instance %i\n", __FUNCTION__, behavior->getBehaviorData()->bdName.c_str(), instanceId);
}

void OGLRenderer::delBehavior(int instanceId) {
  if (mModelInstCamData.micAssimpInstances.size() < instanceId) {
    Logger::log(1, "%s error: number of instances is smaller than instance id %i\n", __FUNCTION__, instanceId);
    return;
  }

  mBehviorTimer.start();
  mBehavior->removeInstance(instanceId);
  mRenderData.rdBehaviorTime += mBehviorTimer.stop();

  //std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(instanceId);
  //instance->isNPC(false);

  Logger::log(1, "%s: removed behavior from instance %i\n", __FUNCTION__, instanceId);
}

void OGLRenderer::addModelBehavior(std::string modelName, std::shared_ptr<SingleInstanceBehavior> behavior) {
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

void OGLRenderer::delModelBehavior(std::string modelName) {
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

void OGLRenderer::updateInstanceSettings(int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting) {
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

void OGLRenderer::addBehaviorEvent(int instanceId, nodeEvent event) {
  mBehavior->addEvent(instanceId, event);
}

void OGLRenderer::postDelNodeTree(std::string nodeTreeName) {
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

void OGLRenderer::updateTriangleCount() {
  mRenderData.rdTriangleCount = 0;
  for (const auto& instance : mModelInstCamData.micAssimpInstances) {
    mRenderData.rdTriangleCount += instance->getModel()->getTriangleCount();
  }
}

void OGLRenderer::enumerateInstances() {
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

void OGLRenderer::cloneCamera() {
  std::shared_ptr<Camera> currentCam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
  std::shared_ptr<Camera> newCam = std::make_shared<Camera>();

  CameraSettings settings = currentCam->getCameraSettings();
  settings.csCamName = generateUniqueCameraName(settings.csCamName);
  newCam->setCameraSettings(settings);

  mModelInstCamData.micCameras.emplace_back(newCam);
  mModelInstCamData.micSelectedCamera = mModelInstCamData.micCameras.size() - 1;
}

void OGLRenderer::deleteCamera() {
  mModelInstCamData.micCameras.erase(mModelInstCamData.micCameras.begin() + mModelInstCamData.micSelectedCamera);
  mModelInstCamData.micSelectedCamera = mModelInstCamData.micCameras.size() - 1;
}

std::string OGLRenderer::generateUniqueCameraName(std::string camBaseName) {
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

bool OGLRenderer::checkCameraNameUsed(std::string cameraName) {
  for (const auto& cam : mModelInstCamData.micCameras) {
    if (cam->getCameraSettings().csCamName == cameraName) {
      return true;
    }
  }

  return false;
}

void OGLRenderer::setSize(unsigned int width, unsigned int height) {
  /* handle minimize */
  if (width == 0 || height == 0) {
    return;
  }

  mRenderData.rdWidth = width;
  mRenderData.rdHeight = height;

  mFramebuffer.resize(width, height);
  glViewport(0, 0, width, height);

  Logger::log(1, "%s: resized window to %dx%d\n", __FUNCTION__, width, height);
}

void OGLRenderer::handleKeyEvents(int key, int scancode, int action, int mods) {
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

void OGLRenderer::setConfigDirtyFlag(bool flag) {
  mConfigIsDirty = flag;
  if (mConfigIsDirty) {
    mWindowTitleDirtySign = "*";
  } else {
    mWindowTitleDirtySign = " ";
  }
  setModeInWindowTitle();
}

bool OGLRenderer::getConfigDirtyFlag() {
  return mConfigIsDirty;
}

void OGLRenderer::setModeInWindowTitle() {
  setWindowTitle(mOrigWindowTitle + " (" + mRenderData.mAppModeMap.at(mRenderData.rdApplicationMode) + " Mode)" + mWindowTitleDirtySign);
}

void OGLRenderer::toggleFullscreen() {
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

void OGLRenderer::checkMouseEnable() {
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

void OGLRenderer::handleMouseButtonEvents(int button, int action, int mods) {
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

void OGLRenderer::handleMousePositionEvents(double xPos, double yPos) {
  /* forward to ImGui only when in edit mode */
  if (mRenderData.rdApplicationMode == appMode::edit) {
    ImGuiIO& io = ImGui::GetIO();

    io.AddMousePosEvent((float)xPos, (float)yPos);

    /* hide from application if above ImGui window */
    if (io.WantCaptureMouse) {
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
    if (mModelInstCamData.micSelectedInstance > 0) {
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

  /* save old values*/
  mMouseXPos = static_cast<int>(xPos);
  mMouseYPos = static_cast<int>(yPos);
}

void OGLRenderer::handleMouseWheelEvents(double xOffset, double yOffset) {
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

void OGLRenderer::handleMovementKeys(float deltaTime) {
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

void OGLRenderer::checkForInstanceCollisions() {
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

      mPerInstanceAnimData.resize(numInstances);

      /* we MUST set the bone offsets to identity matrices to get the skeleton data */
      std::vector<glm::mat4> emptyBoneOfssets(numberOfBones, glm::mat4(1.0f));
      mEmptyBoneOffsetBuffer.uploadSsboData(emptyBoneOfssets);

      /* reusing the array and SSBO for now */
      mWorldPosMatrices.resize(numInstances);

      mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
      mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);

      mBoundingSphereBuffer.checkForResize(numberOfSpheres * sizeof(glm::vec4));

      for (size_t i = 0; i < numInstances; ++i) {
        InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getInstanceSettings();

        PerInstanceAnimData animData{};
        animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
        animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
        animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
        animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
        animData.blendFactor = instSettings.isAnimBlendFactor;

        mPerInstanceAnimData.at(i) = animData;

        mWorldPosMatrices.at(i) = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getWorldTransformMatrix();
      }

      runBoundingSphereComputeShaders(model, numberOfBones, numInstances);

      /* read sphere SSBO per model */
      std::vector<glm::vec4> boundingSpheres = mBoundingSphereBuffer.getSsboDataVec4(numberOfSpheres);

      for (size_t i = 0; i < numInstances; ++i) {
        InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getInstanceSettings();
        int instanceIndex = instSettings.isInstanceIndexPosition;
        mBoundingSpheresPerInstance[instanceIndex].resize(numberOfBones);

        std::copy(boundingSpheres.begin() + i * numberOfBones, boundingSpheres.begin() + (i + 1) * numberOfBones, mBoundingSpheresPerInstance[instanceIndex].begin());
      }
    }

    checkForBoundingSphereCollisions();
  }

  size_t remainingCollisions = mModelInstCamData.micInstanceCollisions.size();

  if (mRenderData.rdDrawBoundingSpheres == collisionDebugDraw::colliding && remainingCollisions > 0) {
    drawCollidingBoundingSpheres();
  }

  /* get (possibly cleaned) number of collisions */
  mRenderData.rdNumberOfCollisions = mModelInstCamData.micInstanceCollisions.size();

  if (mRenderData.rdCheckCollisions != collisionChecks::none) {
    reactToInstanceCollisions();
  }
}

void OGLRenderer::checkForBorderCollisions() {
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

void OGLRenderer::checkForBoundingSphereCollisions() {
  std::set<std::pair<int, int>> sphereCollisions{};

  for (const auto& instancePairs : mModelInstCamData.micInstanceCollisions) {
    int firstId = instancePairs.first;
    int secondId = instancePairs.second;

    /* brute force check of sphere vs sphere */
    bool collisionDetected = false;

    for (size_t first = 0; first < mBoundingSpheresPerInstance[firstId].size(); ++first) {
      glm::vec4 firstSphereData = mBoundingSpheresPerInstance[firstId].at(first);
      float firstRadius = firstSphereData.w;

      /* no need to check disabled spheres*/
      if (firstRadius == 0.0f) {
        continue;
      }

      glm::vec3 firstSpherePos = glm::vec3(firstSphereData.x, firstSphereData.y, firstSphereData.z);

      for (size_t second = 0; second < mBoundingSpheresPerInstance[secondId].size(); ++second) {
        glm::vec4 secondSphereData = mBoundingSpheresPerInstance[secondId].at(second);
        float secondRadius = secondSphereData.w;

        /* no need to check disabled spheres*/
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

void OGLRenderer::reactToInstanceCollisions() {
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

void OGLRenderer::findInteractionInstances() {
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

void OGLRenderer::drawInteractionDebug() {
  if (mModelInstCamData.micSelectedInstance == 0) {
    return;
  }

  glm::vec4 aabbColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

  OGLLineMesh InteractionMesh;
  OGLLineVertex vertex;
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
    vertex.position = glm::vec3(minQueryBoxTopLeft.x, 0.0f, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxTopLeft.x, 0.0f, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(minQueryBoxTopLeft.x, 0.0f, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxBottomRight.x, 0.0f, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(minQueryBoxBottomRight.x, 0.0f, minQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxBottomRight.x, 0.0f, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(minQueryBoxBottomRight.x, 0.0f, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(minQueryBoxTopLeft.x, 0.0f, minQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);

    /* max range */
    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, 0.0f, maxQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, 0.0f, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, 0.0f, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, 0.0f, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, 0.0f, maxQueryBoxBottomRight.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, 0.0f, maxQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);

    vertex.position = glm::vec3(maxQueryBoxBottomRight.x, 0.0f, maxQueryBoxTopLeft.y);
    InteractionMesh.vertices.emplace_back(vertex);
    vertex.position = glm::vec3(maxQueryBoxTopLeft.x, 0.0f, maxQueryBoxTopLeft.y);
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

  if (InteractionMesh.vertices.size() > 0) {
    mUploadToVBOTimer.start();
    mLineVertexBuffer.uploadData(InteractionMesh);
    mRenderData.rdUploadToVBOTime = mUploadToVBOTimer.stop();

    mLineShader.use();
    mLineVertexBuffer.bindAndDraw(GL_LINES, 0, InteractionMesh.vertices.size());
  }

  /* draw instanca AABBs */
  if (mRenderData.rdInteractionCandidates.empty()) {
    return;
  }

  std::vector<std::shared_ptr<AssimpInstance>> instancesToDraw{};
  for (const int id : mRenderData.rdInteractionCandidates) {
    instancesToDraw.emplace_back(mModelInstCamData.micAssimpInstances.at(id));
  }

  drawAABBs(instancesToDraw, aabbColor);
}

void OGLRenderer::drawAABBs(std::vector<std::shared_ptr<AssimpInstance>> instances, glm::vec4 aabbColor) {
  std::shared_ptr<OGLLineMesh> aabbLineMesh = nullptr;;

  mAABBMesh->vertices.clear();
  AABB instanceAABB;
  mAABBMesh->vertices.resize(instances.size() * instanceAABB.getAABBLines(aabbColor)->vertices.size());

  for (size_t i = 0; i < instances.size(); ++i) {
    InstanceSettings instSettings = instances.at(i)->getInstanceSettings();

    /* skip null instance*/
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

  mUploadToVBOTimer.start();
  mLineVertexBuffer.uploadData(*mAABBMesh);
  mRenderData.rdUploadToVBOTime = mUploadToVBOTimer.stop();

  if (mAABBMesh->vertices.size() > 0) {
    mLineShader.use();
    mLineVertexBuffer.bindAndDraw(GL_LINES, 0, mAABBMesh->vertices.size());
  }
}

void OGLRenderer::drawCollisionDebug() {
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
    /* draw colliding instances in res */
    if (mRenderData.rdDrawCollisionAABBs == collisionDebugDraw::colliding ||
        mRenderData.rdDrawCollisionAABBs == collisionDebugDraw::all) {
      for (const auto id : uniqueInstanceIds){
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

  /* no bounding sphere collision will be done with this setting, so run the computer shaders just for the selected instance */
  if (mRenderData.rdDrawBoundingSpheres == collisionDebugDraw::selected) {
    drawSelectedBoundingSpheres();
  }

  if (mRenderData.rdDrawBoundingSpheres == collisionDebugDraw::all) {
    drawAllBoundingSpheres();
  }
}


void OGLRenderer::drawSelectedBoundingSpheres() {
  mCollisionDebugDrawTimer.start();

  if (mModelInstCamData.micSelectedInstance > 0 ) {
    std::shared_ptr<AssimpInstance> instance = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance);
    std::shared_ptr<AssimpModel> model = instance->getModel();

    size_t numberOfBones = model->getBoneList().size();

    size_t numberOfSpheres = numberOfBones;
    size_t trsMatrixSize = numberOfBones * sizeof(glm::mat4);

    mPerInstanceAnimData.resize(1);

    /* we MUST set the bone offsets to identity matrices to get the skeleton data */
    std::vector<glm::mat4> emptyBoneOfssets(numberOfBones, glm::mat4(1.0f));
    mEmptyBoneOffsetBuffer.uploadSsboData(emptyBoneOfssets);

    /* reusing the array and SSBO for now */
    mWorldPosMatrices.resize(1);

    mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
    mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);

    mBoundingSphereBuffer.checkForResize(numberOfSpheres * sizeof(glm::vec4));
    InstanceSettings instSettings = instance->getInstanceSettings();

    PerInstanceAnimData animData{};
    animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
    animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
    animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
    animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
    animData.blendFactor = instSettings.isAnimBlendFactor;

    mPerInstanceAnimData.at(0) = animData;

    mWorldPosMatrices.at(0) = instance->getWorldTransformMatrix();

    runBoundingSphereComputeShaders(model, numberOfBones, 1);

    mUploadToVBOTimer.start();
    mLineVertexBuffer.uploadData(mSphereMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();

    if (numberOfSpheres > 0) {
      mSphereShader.use();
      mBoundingSphereBuffer.bind(1);
      mLineVertexBuffer.bindAndDrawInstanced(GL_LINES, 0, mSphereMesh.vertices.size(), numberOfSpheres);
    }
  }

  mRenderData.rdCollisionDebugDrawTime += mCollisionDebugDrawTimer.stop();
}

void OGLRenderer::drawCollidingBoundingSpheres() {
  mCollisionDebugDrawTimer.start();

  /* split instances in models - use a std::set to get unique instance IDs */
  std::map<std::string, std::set<int>> modelToInstanceMapping;

  for (const auto& instancePairs : mModelInstCamData.micInstanceCollisions) {
    modelToInstanceMapping[mModelInstCamData.micAssimpInstances.at(instancePairs.first)->getModel()->getModelFileName()].insert(instancePairs.first);
    modelToInstanceMapping[mModelInstCamData.micAssimpInstances.at(instancePairs.second)->getModel()->getModelFileName()].insert(instancePairs.second);
  }
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

    mPerInstanceAnimData.resize(numInstances);

    /* we MUST set the bone offsets to identity matrices to get the skeleton data */
    std::vector<glm::mat4> emptyBoneOfssets(numberOfBones, glm::mat4(1.0f));
    mEmptyBoneOffsetBuffer.uploadSsboData(emptyBoneOfssets);

    /* reusing the array and SSBO for now */
    mWorldPosMatrices.resize(numInstances);

    mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
    mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);

    mBoundingSphereBuffer.checkForResize(numberOfSpheres * sizeof(glm::vec4));

    for (size_t i = 0; i < instanceIds.size(); ++i) {
      InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getInstanceSettings();

      PerInstanceAnimData animData{};
      animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
      animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
      animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
      animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
      animData.blendFactor = instSettings.isAnimBlendFactor;

      mPerInstanceAnimData.at(i) = animData;

      mWorldPosMatrices.at(i) = mModelInstCamData.micAssimpInstances.at(instanceIds.at(i))->getWorldTransformMatrix();
    }

    runBoundingSphereComputeShaders(model, numberOfBones, numInstances);

    mUploadToVBOTimer.start();
    mLineVertexBuffer.uploadData(mCollidingSphereMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();

    if (numberOfSpheres > 0) {
      mSphereShader.use();
      mBoundingSphereBuffer.bind(1);
      mLineVertexBuffer.bindAndDrawInstanced(GL_LINES, 0, mCollidingSphereMesh.vertices.size(), numberOfSpheres);
    }
  }
  mRenderData.rdCollisionDebugDrawTime += mCollisionDebugDrawTimer.stop();
}

void OGLRenderer::drawAllBoundingSpheres() {
  mCollisionDebugDrawTimer.start();

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

    mPerInstanceAnimData.resize(numInstances);

    /* we MUST set the bone offsets to identity matrices to get the skeleton data */
    std::vector<glm::mat4> emptyBoneOfssets(numberOfBones, glm::mat4(1.0f));
    mEmptyBoneOffsetBuffer.uploadSsboData(emptyBoneOfssets);

    /* reusing the array and SSBO for now */
    mWorldPosMatrices.resize(numInstances);

    mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
    mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);

    mBoundingSphereBuffer.checkForResize(numberOfSpheres * sizeof(glm::vec4));

    for (int i = 0; i < numInstances; ++i) {
      InstanceSettings instSettings = instances.at(i)->getInstanceSettings();

      PerInstanceAnimData animData{};
      animData.firstAnimClipNum = instSettings.isFirstAnimClipNr;
      animData.secondAnimClipNum = instSettings.isSecondAnimClipNr;
      animData.firstClipReplayTimestamp = instSettings.isFirstClipAnimPlayTimePos;
      animData.secondClipReplayTimestamp = instSettings.isSecondClipAnimPlayTimePos;
      animData.blendFactor = instSettings.isAnimBlendFactor;

      mPerInstanceAnimData.at(i) = animData;

      mWorldPosMatrices.at(i) = instances.at(i)->getWorldTransformMatrix();
    }

    runBoundingSphereComputeShaders(model, numberOfBones, numInstances);

    mUploadToVBOTimer.start();
    mLineVertexBuffer.uploadData(mSphereMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();

    if (numberOfSpheres > 0) {
      mSphereShader.use();
      mBoundingSphereBuffer.bind(1);
      mLineVertexBuffer.bindAndDrawInstanced(GL_LINES, 0, mSphereMesh.vertices.size(), numberOfSpheres);
    }
  }

  mRenderData.rdCollisionDebugDrawTime += mCollisionDebugDrawTimer.stop();
}

void OGLRenderer::runBoundingSphereComputeShaders(std::shared_ptr<AssimpModel> model, int numberOfBones, int numInstances) {
  ModelSettings modSettings = model->getModelSettings();

  /* do a single iteration of all clips in parallel */
  mAssimpTransformComputeShader.use();

  mUploadToUBOTimer.start();
  model->bindAnimLookupBuffer(0);
  mPerInstanceAnimDataBuffer.uploadSsboData(mPerInstanceAnimData, 1);
  mShaderTRSMatrixBuffer.bind(2);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  glDispatchCompute(numberOfBones, std::ceil(numInstances/32.0f), 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


  mAssimpMatrixComputeShader.use();

  mUploadToUBOTimer.start();
  mShaderTRSMatrixBuffer.bind(0);
  model->bindBoneParentBuffer(1);
  mEmptyBoneOffsetBuffer.bind(2);
  mShaderBoneMatrixBuffer.bind(3);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  glDispatchCompute(numberOfBones, std::ceil(numInstances/32.0f), 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);


  /* calculate sphere center per bone and radius in a shader (too much for CPU work) */
  mAssimpBoundingBoxComputeShader.use();

  mUploadToUBOTimer.start();
  mShaderBoneMatrixBuffer.bind(0);
  mShaderModelRootMatrixBuffer.uploadSsboData(mWorldPosMatrices, 1);
  model->bindBoneParentBuffer(2);
  mBoundingSphereAdjustmentBuffer.uploadSsboData(modSettings.msBoundingSphereAdjustments, 3);
  mBoundingSphereBuffer.bind(4);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  /* run only for the number of spheres we have, avoid buffer overwrites */
  glDispatchCompute(numberOfBones, std::ceil(numInstances/32.0f), 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

bool OGLRenderer::draw(float deltaTime) {
  if (!mApplicationRunning) {
    return false;
  }

  /* no update on zero diff */
  if (deltaTime == 0.0f) {
    return true;
  }

  /* handle minimize */
  while (mRenderData.rdWidth == 0 || mRenderData.rdHeight == 0) {
    glfwGetFramebufferSize(mRenderData.rdWindow, &mRenderData.rdWidth, &mRenderData.rdHeight);
    glfwWaitEvents();
  }

  mRenderData.rdFrameTime = mFrameTimer.stop();
  mFrameTimer.start();

  handleMovementKeys(deltaTime);

  std::shared_ptr<Camera> cam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
  CameraSettings camSettings = cam->getCameraSettings();

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

  /* draw to framebuffer */
  mFramebuffer.bind();
  mFramebuffer.clearTextures();

  /* camera update */
  mMatrixGenerateTimer.start();
  cam->updateCamera(mRenderData, deltaTime);

  if (camSettings.csCamProjection == cameraProjection::perspective) {
    mProjectionMatrix = glm::perspective(
      glm::radians(static_cast<float>(camSettings.csFieldOfView)),
      static_cast<float>(mRenderData.rdWidth) / static_cast<float>(mRenderData.rdHeight),
      0.1f, 500.0f);
  } else {
    float orthoScaling = camSettings.csOrthoScale;
    float aspect = static_cast<float>(mRenderData.rdWidth) / static_cast<float>(mRenderData.rdHeight) * orthoScaling;
    float leftRight = 1.0f * orthoScaling;
    float nearFar = 75.0f * orthoScaling;
    mProjectionMatrix = glm::ortho(-aspect, aspect, -leftRight, leftRight, -nearFar, nearFar);
  }

  mViewMatrix = cam->getViewMatrix();

  mRenderData.rdMatrixGenerateTime = mMatrixGenerateTimer.stop();

  mUploadToUBOTimer.start();
  std::vector<glm::mat4> matrixData;
  matrixData.emplace_back(mViewMatrix);
  matrixData.emplace_back(mProjectionMatrix);
  mUniformBuffer.uploadUboData(matrixData, 0);
  mRenderData.rdUploadToUBOTime = mUploadToUBOTimer.stop();


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

  mRenderData.rdMatricesSize = 0;
  mRenderData.rdNumberOfCollisions = 0;
  mRenderData.rdCollisionDebugDrawTime = 0.0f;
  mRenderData.rdCollisionCheckTime = 0.0f;
  mRenderData.rdBehaviorTime = 0.0f;
  mRenderData.rdNumberOfInteractionCandidates = 0;
  mRenderData.rdInteractWithInstanceId = 0;
  mRenderData.rdFaceAnimTime = 0.0f;

  mQuadtree->clear();

  for (const auto& model : mModelInstCamData.micModelList) {
    size_t numberOfInstances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() &&
        model->getBoneList().size() > 0) {

        size_t numberOfBones = model->getBoneList().size();
        ModelSettings modSettings = model->getModelSettings();

        mMatrixGenerateTimer.start();

        mPerInstanceAnimData.resize(numberOfInstances);
        mPerInstanceAABB.resize(numberOfInstances);
        mWorldPosMatrices.resize(numberOfInstances);
        mSelectedInstance.resize(numberOfInstances);

        mFaceAnimPerInstanceData.resize(numberOfInstances);

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()];
        for (size_t i = 0; i < numberOfInstances; ++i) {
          instances.at(i)->updateAnimation(deltaTime);
          instances.at(i)->updateInstanceSpeed(deltaTime);
          instances.at(i)->updateInstancePosition(deltaTime);

          mWorldPosMatrices.at(i) = instances.at(i)->getWorldTransformMatrix();

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

          mPerInstanceAnimData.at(i) = animData;

          if (mRenderData.rdApplicationMode == appMode::edit) {
            if (currentSelectedInstance == instances.at(i)) {
              mSelectedInstance.at(i).x = mRenderData.rdSelectedInstanceHighlightValue;
            } else {
              mSelectedInstance.at(i).x = 1.0f;
            }

            if (mMousePick) {
              mSelectedInstance.at(i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
            }
          } else {
            mSelectedInstance.at(i).x = 1.0f;
          }

          /* get AABB and calculate 2D boundaries */
          AABB instanceAABB = model->getAABB(instSettings);

          glm::vec2 position = glm::vec2(instanceAABB.getMinPos().x, instanceAABB.getMinPos().z);
          glm::vec2 size = glm::vec2(std::fabs(instanceAABB.getMaxPos().x - instanceAABB.getMinPos().x),
                                     std::fabs(instanceAABB.getMaxPos().z - instanceAABB.getMinPos().z));

          BoundingBox2D box{position, size};
          instances.at(i)->setBoundingBox(box);

          /* add instance to quadtree*/
          mQuadtree->add(instSettings.isInstanceIndexPosition);

          /* use a glm::vec3 to transport all morph data */
          mFaceAnimTimer.start();

          glm::vec4 morphData = glm::vec4(0.0f);
          if (instSettings.isFaceAnim != faceAnimation::none)  {
            morphData.x = instSettings.isFaceAnimWeight;
            morphData.y = static_cast<int>(instSettings.isFaceAnim) - 1;
            morphData.z = model->getAnimMeshVertexSize();
          }
          mFaceAnimPerInstanceData.at(i) = morphData;

          mRenderData.rdFaceAnimTime += mFaceAnimTimer.stop();
        }

        size_t trsMatrixSize = numberOfBones * numberOfInstances * sizeof(glm::mat4);
        mRenderData.rdMatricesSize += trsMatrixSize;

        /* we may have to resize the buffers (uploadSsboData() checks for the size automatically, bind() not) */
        mShaderBoneMatrixBuffer.checkForResize(trsMatrixSize);
        mShaderTRSMatrixBuffer.checkForResize(trsMatrixSize);

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();

        /* calculate TRS matrices from node transforms */
        if (model->hasHeadMovementAnimationsMapped()) {
          mAssimpTransformHeadMoveComputeShader.use();
        } else {
          mAssimpTransformComputeShader.use();
        }

        mUploadToUBOTimer.start();
        model->bindAnimLookupBuffer(0);
        mPerInstanceAnimDataBuffer.uploadSsboData(mPerInstanceAnimData, 1);
        mShaderTRSMatrixBuffer.bind(2);

        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        /* do the computation - in groups of 32 invocations */
        glDispatchCompute(numberOfBones, std::ceil(numberOfInstances / 32.0f), 1);
        //glDispatchCompute(numberOfBones, numberOfInstances, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* multiply every bone TRS matrix with its parent bones TRS matrices, until the root bone has been reached
         * also, multiply the bone TRS and the bone offset matrix */
        mAssimpMatrixComputeShader.use();

        mUploadToUBOTimer.start();
        mShaderTRSMatrixBuffer.bind(0);
        model->bindBoneParentBuffer(1);
        model->bindBoneMatrixOffsetBuffer(2);
        mShaderBoneMatrixBuffer.bind(3);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        /* do the computation - in groups of 32 invocations */
        glDispatchCompute(numberOfBones, std::ceil(numberOfInstances / 32.0f), 1);
        //glDispatchCompute(numberOfBones, numberOfInstances, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        /* get the bone matrix of the selected bone from the SSBO */
        std::shared_ptr<Camera> cam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
        CameraSettings camSettings = cam->getCameraSettings();

        if (camSettings.csCamType == cameraType::firstPerson && cam->getInstanceToFollow() &&
          model == cam->getInstanceToFollow()->getModel()) {
          int selectedInstance = cam->getInstanceToFollow()->getInstanceSettings().isInstancePerModelIndexPosition;
          int selectedBone = camSettings.csFirstPersonBoneToFollow;
          glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), camSettings.csFirstPersonOffsets);
          glm::mat4 boneMatrix = mShaderBoneMatrixBuffer.getSsboDataMat4(selectedInstance * numberOfBones + selectedBone, 1).at(0);

          cam->setBoneMatrix(mWorldPosMatrices.at(selectedInstance) * boneMatrix * offsetMatrix *
            glm::inverse(model->getBoneList().at(selectedBone)->getOffsetMatrix()));

          cam->setCameraSettings(camSettings);
        }

        /* now bind the final bone transforms to the vertex skinning shader */
        if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
          mAssimpSkinningSelectionShader.use();
        } else {
          mAssimpSkinningShader.use();
        }

        mUploadToUBOTimer.start();

        mAssimpSkinningShader.setUniformValue(numberOfBones);
        mShaderBoneMatrixBuffer.bind(1);
        mShaderModelRootMatrixBuffer.uploadSsboData(mWorldPosMatrices, 2);
        mSelectedInstanceBuffer.uploadSsboData(mSelectedInstance, 3);

        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        model->drawInstancedNoMorphAnims(numberOfInstances);

        if (model->hasAnimMeshes()) {
          mFaceAnimTimer.start();

          if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
            mAssimpSkinningMorphSelectionShader.use();
          } else {
            mAssimpSkinningMorphShader.use();
          }

          mUploadToUBOTimer.start();

          mAssimpSkinningMorphShader.setUniformValue(numberOfBones);
          mShaderBoneMatrixBuffer.bind(1);
          mShaderModelRootMatrixBuffer.bind(2);
          mSelectedInstanceBuffer.bind(3);
          model->bindMorphAnimBuffer(4);
          mFaceAnimPerInstanceDataBuffer.uploadSsboData(mFaceAnimPerInstanceData, 5);

          mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

          model->drawInstancedMorphAnims(numberOfInstances);

          mRenderData.rdFaceAnimTime += mFaceAnimTimer.stop();
        }
      } else {
        /* non-animated models */

        mMatrixGenerateTimer.start();
        mWorldPosMatrices.resize(numberOfInstances);
        mSelectedInstance.resize(numberOfInstances);

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstCamData.micAssimpInstancesPerModel[model->getModelFileName()];

        for (size_t i = 0; i < numberOfInstances; ++i) {
          mWorldPosMatrices.at(i) = instances.at(i)->getWorldTransformMatrix();

          if (mRenderData.rdApplicationMode == appMode::edit) {
            if (currentSelectedInstance == instances.at(i)) {
              mSelectedInstance.at(i).x = mRenderData.rdSelectedInstanceHighlightValue;
            } else {
              mSelectedInstance.at(i).x = 1.0f;
            }

            if (mMousePick) {
              InstanceSettings instSettings = instances.at(i)->getInstanceSettings();
              mSelectedInstance.at(i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
            }
          } else {
            mSelectedInstance.at(i).x = 1.0f;
          }
        }

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
        mRenderData.rdMatricesSize += mWorldPosMatrices.size() * sizeof(glm::mat4);

        /* get the bone matrix of the selected bone from the SSBO */
        std::shared_ptr<Camera> cam = mModelInstCamData.micCameras.at(mModelInstCamData.micSelectedCamera);
        CameraSettings camSettings = cam->getCameraSettings();

        if (camSettings.csCamType == cameraType::firstPerson && cam->getInstanceToFollow() &&
          model == cam->getInstanceToFollow()->getModel()) {
          int selectedInstance = cam->getInstanceToFollow()->getInstanceSettings().isInstancePerModelIndexPosition;
          glm::mat4 offsetMatrix = glm::translate(glm::mat4(1.0f), camSettings.csFirstPersonOffsets);

          cam->setBoneMatrix(mWorldPosMatrices.at(selectedInstance) * offsetMatrix);

          cam->setCameraSettings(camSettings);
        }

        if (mMousePick && mRenderData.rdApplicationMode == appMode::edit) {
          mAssimpSelectionShader.use();
        } else {
          mAssimpShader.use();
        }

        mUploadToUBOTimer.start();
        mShaderModelRootMatrixBuffer.uploadSsboData(mWorldPosMatrices, 1);
        mSelectedInstanceBuffer.uploadSsboData(mSelectedInstance, 2);

        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        model->drawInstanced(numberOfInstances);
      }
    }
  }

  /* draw coord arrow, depending on edit mode */
  mCoordArrowsLineIndexCount = 0;
  mLineMesh->vertices.clear();
  if (mRenderData.rdApplicationMode == appMode::edit) {

    if (mMousePick) {
      /* wait until selection buffer has been filled */
      glFlush();
      glFinish();

      /* inverted Y */
      float selectedInstanceId = mFramebuffer.readPixelFromPos(mMouseXPos, (mRenderData.rdHeight - mMouseYPos - 1));

      if (selectedInstanceId >= 0.0f) {
        mModelInstCamData.micSelectedInstance = static_cast<int>(selectedInstanceId);
      } else {
        mModelInstCamData.micSelectedInstance = 0;
      }
      mModelInstCamData.micSettingsContainer->applySelectInstance(mModelInstCamData.micSelectedInstance, mSavedSelectedInstanceId);
      mMousePick = false;
    }

    if (mModelInstCamData.micSelectedInstance > 0) {
      InstanceSettings instSettings = mModelInstCamData.micAssimpInstances.at(mModelInstCamData.micSelectedInstance)->getInstanceSettings();

      /* draw coordiante arrows at origin of selected instance*/
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

      mCoordArrowsLineIndexCount += mCoordArrowsMesh.vertices.size();
      std::for_each(mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end(),
        [=](auto &n){
          n.color /= 2.0f;
          n.position = glm::quat(glm::radians(instSettings.isWorldRotation)) * n.position;
          n.position += instSettings.isWorldPosition;
      });
      mLineMesh->vertices.insert(mLineMesh->vertices.end(),
                                 mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end());
    }

    mUploadToVBOTimer.start();
    mLineVertexBuffer.uploadData(*mLineMesh);
    mRenderData.rdUploadToVBOTime += mUploadToVBOTimer.stop();

    /* draw the coordinate arrow WITH depth buffer */
    if (mCoordArrowsLineIndexCount > 0) {
      mLineShader.use();
      mLineVertexBuffer.bindAndDraw(GL_LINES, 0, mCoordArrowsLineIndexCount);
    }
  }

  mInteractionTimer.start();
  findInteractionInstances();
  drawInteractionDebug();
  mRenderData.rdInteractionTime = mInteractionTimer.stop();

  /* check for collisions */
  mCollisionCheckTimer.start();
  checkForInstanceCollisions();
  checkForBorderCollisions();
  mRenderData.rdCollisionCheckTime += mCollisionCheckTimer.stop();

  mCollisionDebugDrawTimer.start();
  drawCollisionDebug();
  mRenderData.rdCollisionDebugDrawTime += mCollisionDebugDrawTimer.stop();

  /* behavior update */
  mBehviorTimer.start();
  mBehavior->update(deltaTime);
  mRenderData.rdBehaviorTime += mBehviorTimer.stop();

  mFramebuffer.unbind();

  /* blit color buffer to screen */
  /* XXX: enable sRGB ONLY for the final framebuffer draw */
  glEnable(GL_FRAMEBUFFER_SRGB);
  mFramebuffer.drawToScreen();
  glDisable(GL_FRAMEBUFFER_SRGB);

  /* create user interface */
  mUIGenerateTimer.start();
  mUserInterface.createFrame(mRenderData);

  if (mRenderData.rdApplicationMode != appMode::view) {
    mUserInterface.hideMouse(mMouseLock);
    mUserInterface.createSettingsWindow(mRenderData, mModelInstCamData);
  }

  /* always draw the status bar and instance positions window */
  mUserInterface.createStatusBar(mRenderData, mModelInstCamData);
  mUserInterface.createPositionsWindow(mRenderData, mModelInstCamData);

  /* only loaded data right now */
  if (mGraphEditor->getShowEditor()) {
    mGraphEditor->updateGraphNodes(deltaTime);
  }

  if (mRenderData.rdApplicationMode != appMode::view) {
    mGraphEditor->createNodeEditorWindow(mRenderData, mModelInstCamData);
  }

  mRenderData.rdUIGenerateTime = mUIGenerateTimer.stop();

  mUIDrawTimer.start();
  mUserInterface.render();
  mRenderData.rdUIDrawTime = mUIDrawTimer.stop();

  return true;
}

void OGLRenderer::cleanup() {
  mShaderModelRootMatrixBuffer.cleanup();
  mSelectedInstanceBuffer.cleanup();
  mShaderBoneMatrixBuffer.cleanup();
  mPerInstanceAnimDataBuffer.cleanup();
  mEmptyBoneOffsetBuffer.cleanup();
  mBoundingSphereBuffer.cleanup();
  mBoundingSphereAdjustmentBuffer.cleanup();
  mShaderTRSMatrixBuffer.cleanup();

  mAssimpTransformHeadMoveComputeShader.cleanup();
  mAssimpTransformComputeShader.cleanup();
  mAssimpMatrixComputeShader.cleanup();
  mAssimpBoundingBoxComputeShader.cleanup();

  mAssimpSkinningMorphSelectionShader.cleanup();
  mAssimpSkinningSelectionShader.cleanup();
  mAssimpSkinningMorphShader.cleanup();
  mAssimpSelectionShader.cleanup();
  mAssimpSkinningShader.cleanup();
  mAssimpShader.cleanup();
  mSphereShader.cleanup();
  mLineShader.cleanup();

  mUserInterface.cleanup();

  mLineVertexBuffer.cleanup();
  mUniformBuffer.cleanup();

  mFramebuffer.cleanup();
}
