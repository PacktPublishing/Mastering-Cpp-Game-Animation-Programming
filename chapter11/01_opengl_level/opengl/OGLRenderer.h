#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <functional>
#include <chrono>

#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Timer.h"
#include "Framebuffer.h"
#include "LineVertexBuffer.h"
#include "Texture.h"
#include "Shader.h"
#include "UniformBuffer.h"
#include "ShaderStorageBuffer.h"
#include "UserInterface.h"
#include "Camera.h"
#include "CoordArrowsModel.h"
#include "RotationArrowsModel.h"
#include "ScaleArrowsModel.h"
#include "SphereModel.h"
#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "YamlParser.h"
#include "Octree.h"
#include "GraphEditor.h"
#include "Behavior.h"
#include "AssimpLevel.h"

#include "OGLRenderData.h"
#include "ModelInstanceCamData.h"

using GetWindowTitleCallback = std::function<std::string(void)>;
using SetWindowTitleCallback = std::function<void(std::string)>;

class OGLRenderer {
  public:
    OGLRenderer(GLFWwindow *window);

    bool init(unsigned int width, unsigned int height);
    void setSize(unsigned int width, unsigned int height);
    void uploadAssimpData(OGLMesh vertexData);
    bool draw(float deltaTime);
    void handleKeyEvents(int key, int scancode, int action, int mods);
    void handleMouseButtonEvents(int button, int action, int mods);
    void handleMousePositionEvents(double xPos, double yPos);
    void handleMouseWheelEvents(double xOffset, double yOffset);

    void cleanup();

    void addNullModelAndInstance();
    void removeAllModelsAndInstances();

    bool hasModel(std::string modelFileName);
    std::shared_ptr<AssimpModel> getModel(std::string modelFileName);
    bool addModel(std::string modelFileName, bool addInitialInstance = true, bool withUndo = true);
    void addExistingModel(std::shared_ptr<AssimpModel> model, int indexPos);
    void deleteModel(std::string modelFileName, bool withUnd = true);

    std::shared_ptr<AssimpInstance> addInstance(std::shared_ptr<AssimpModel> model, bool withUndo = true);
    void addExistingInstance(std::shared_ptr<AssimpInstance> instance, int indexPos, int indexPerModelPos);
    void addInstances(std::shared_ptr<AssimpModel> model, int numInstances);
    void deleteInstance(std::shared_ptr<AssimpInstance> instance, bool withUndo = true);
    void cloneInstance(std::shared_ptr<AssimpInstance> instance);
    void cloneInstances(std::shared_ptr<AssimpInstance> instance, int numClones);
    std::shared_ptr<AssimpInstance> getInstanceById(int instanceId);

    void centerInstance(std::shared_ptr<AssimpInstance> instance);

    void addBehavior(int instanceId, std::shared_ptr<SingleInstanceBehavior> behavior);
    void delBehavior(int instanceId);
    void postDelNodeTree(std::string nodeTreeName);
    void updateInstanceSettings(int instanceId, graphNodeType nodeType, instanceUpdateType updateType, nodeCallbackVariant data, bool extraSetting);
    void addBehaviorEvent(int instanceId, nodeEvent event);

    void addModelBehavior(std::string modelName, std::shared_ptr<SingleInstanceBehavior> behavior);
    void delModelBehavior(std::string modelName);

    bool hasLevel(std::string levelFileName);
    std::shared_ptr<AssimpLevel> getLevel(std::string levelFileName);
    bool addLevel(std::string levelFileName);
    void deleteLevel(std::string levelFileName);

    void addNullLevel();

    void requestExitApplication();
    void doExitApplication();

    SetWindowTitleCallback setWindowTitle;
    GetWindowTitleCallback getWindowTitle;

    std::shared_ptr<BoundingBox3D> getWorldBoundaries();

  private:
    OGLRenderData mRenderData{};
    ModelInstanceCamData mModelInstCamData{};

    Timer mFrameTimer{};
    Timer mMatrixGenerateTimer{};
    Timer mIKTimer{};
    Timer mUploadToVBOTimer{};
    Timer mUploadToUBOTimer{};
    Timer mUIGenerateTimer{};
    Timer mUIDrawTimer{};
    Timer mCollisionDebugDrawTimer{};
    Timer mCollisionCheckTimer{};
    Timer mBehviorTimer{};
    Timer mInteractionTimer{};
    Timer mFaceAnimTimer{};

    Shader mLineShader{};
    Shader mSphereShader{};
    Shader mAssimpShader{};
    Shader mAssimpSkinningShader{};
    Shader mAssimpSkinningMorphShader{};

    Shader mAssimpSelectionShader{};
    Shader mAssimpSkinningSelectionShader{};
    Shader mAssimpSkinningMorphSelectionShader{};

    Shader mAssimpTransformComputeShader{};
    Shader mAssimpTransformHeadMoveComputeShader{};
    Shader mAssimpMatrixComputeShader{};
    Shader mAssimpBoundingBoxComputeShader{};

    Shader mAssimpLevelShader{};

    Framebuffer mFramebuffer{};
    LineVertexBuffer mLineVertexBuffer{};
    UniformBuffer mUniformBuffer{};
    UserInterface mUserInterface{};

    /* for animated and non-animated models */
    ShaderStorageBuffer mShaderModelRootMatrixBuffer{};
    std::vector<glm::mat4> mWorldPosMatrices{};

    /* color hightlight for selection etc */
    std::vector<glm::vec2> mSelectedInstance{};
    ShaderStorageBuffer mSelectedInstanceBuffer{};

    /* for animated models */
    ShaderStorageBuffer mShaderBoneMatrixBuffer{};
    std::vector<PerInstanceAnimData> mPerInstanceAnimData{};
    ShaderStorageBuffer mPerInstanceAnimDataBuffer{};
    ShaderStorageBuffer mEmptyBoneOffsetBuffer{};

    /* x/y/z is shpere center, w is radius*/
    ShaderStorageBuffer mBoundingSphereBuffer{};
    /* per-model-and-node adjustments for the spheres */
    ShaderStorageBuffer mBoundingSphereAdjustmentBuffer{};

    std::vector<AABB> mPerInstanceAABB{};
    std::shared_ptr<OGLLineMesh> mAABBMesh = nullptr;

    /* for compute shader */
    ShaderStorageBuffer mShaderTRSMatrixBuffer{};

    CoordArrowsModel mCoordArrowsModel{};
    RotationArrowsModel mRotationArrowsModel{};
    ScaleArrowsModel mScaleArrowsModel{};

    OGLLineMesh mCoordArrowsMesh{};
    std::shared_ptr<OGLLineMesh> mLineMesh = nullptr;

    SphereModel mSphereModel{};
    SphereModel mCollidingSphereModel{};
    OGLLineMesh mSphereMesh{};
    OGLLineMesh mCollidingSphereMesh{};

    unsigned int mCoordArrowsLineIndexCount = 0;

    bool mMouseLock = false;
    int mMouseXPos = 0;
    int mMouseYPos = 0;
    float mMouseWheelScale = 1.0f;
    int mMouseWheelScaleShiftKey = 0;
    bool mMouseWheelScrolling = false;
    std::chrono::time_point<std::chrono::steady_clock> mMouseWheelLastScrollTime;
    CameraSettings mSavedCameraWheelSettings;

    bool mMousePick = false;
    int mSavedSelectedInstanceId = 0;

    bool mMouseMove = false;
    bool mMouseMoveVertical = false;
    int mMouseMoveVerticalShiftKey = 0;
    InstanceSettings mSavedInstanceSettings;

    void handleMovementKeys(float deltaTime);

    void updateTriangleCount();
    void updateLevelTriangleCount();
    void enumerateInstances();

    /* create identity matrix by default */
    glm::mat4 mViewMatrix = glm::mat4(1.0f);
    glm::mat4 mProjectionMatrix = glm::mat4(1.0f);

    std::string mOrigWindowTitle;
    void setModeInWindowTitle();

    void toggleFullscreen();
    void checkMouseEnable();

    bool mApplicationRunning = false;

    void undoLastOperation();
    void redoLastOperation();

    void createSettingsContainerCallbacks();
    void clearUndoRedoStacks();

    const std::string mDefaultConfigFileName = "config/conf.acfg";
    bool loadConfigFile(std::string configFileName);
    bool saveConfigFile(std::string configFileName);
    void createEmptyConfig();

    void loadDefaultFreeCam();

    bool mConfigIsDirty = false;
    std::string mWindowTitleDirtySign;
    void setConfigDirtyFlag(bool flag);
    bool getConfigDirtyFlag();

    void cloneCamera();
    void deleteCamera();
    CameraSettings mSavedCameraSettings;

    std::string generateUniqueCameraName(std::string camBaseName);
    bool checkCameraNameUsed(std::string cameraName);

    std::vector<glm::vec3> getPositionOfAllInstances();

    void initOctree(int thresholdPerBox, int maxDepth);
    std::shared_ptr<Octree> mOctree = nullptr;
    std::shared_ptr<BoundingBox3D> mWorldBoundaries = nullptr;

    void createAABBLookup(std::shared_ptr<AssimpModel> model);
    void drawAABBs(std::vector<std::shared_ptr<AssimpInstance>> instances, glm::vec4 aabbColor);
    void drawCollisionDebug();
    void drawSelectedBoundingSpheres();
    void drawCollidingBoundingSpheres();
    void drawAllBoundingSpheres();

    void runBoundingSphereComputeShaders(std::shared_ptr<AssimpModel> model, int numberOfBones, int numInstances);

    std::map<int, std::vector<glm::vec4>> mBoundingSpheresPerInstance;

    void checkForInstanceCollisions();
    void checkForBorderCollisions();
    void checkForBoundingSphereCollisions();
    void reactToInstanceCollisions();

    void findInteractionInstances();
    void drawInteractionDebug();

    void resetCollisionData();

    std::shared_ptr<GraphEditor> mGraphEditor = nullptr;
    void editGraph(std::string graphName);
    std::shared_ptr<SingleInstanceBehavior> createEmptyGraph();

    std::shared_ptr<Behavior> mBehavior = nullptr;
    instanceNodeActionCallback mInstanceNodeActionCallback;

    std::vector<glm::vec4> mFaceAnimPerInstanceData{};
    ShaderStorageBuffer mFaceAnimPerInstanceDataBuffer{};

    void generateLevelAABB();
    void drawLevelAABB(glm::vec4 aabbColor);
    AABB mAllLevelAABB{};
};
