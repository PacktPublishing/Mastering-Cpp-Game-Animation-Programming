/* Vulkan renderer */
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <map>
#include <chrono>
#include <random>

#include <glm/glm.hpp>

/* Vulkan also before GLFW */
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include "Timer.h"
#include "Texture.h"
#include "UniformBuffer.h"
#include "UserInterface.h"
#include "VertexBuffer.h"
#include "ShaderStorageBuffer.h"
#include "CameraSettings.h"
#include "ModelSettings.h"
#include "CoordArrowsModel.h"
#include "RotationArrowsModel.h"
#include "ScaleArrowsModel.h"
#include "SphereModel.h"
#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "Octree.h"
#include "BoundingBox3D.h"
#include "TriangleOctree.h"
#include "GraphEditor.h"
#include "SingleInstanceBehavior.h"
#include "Behavior.h"
#include "Callbacks.h"
#include "AssimpLevel.h"
#include "IKSolver.h"
#include "PathFinder.h"
#include "SkyboxModel.h"

#include "VkRenderData.h"
#include "ModelInstanceCamData.h"

#include "Callbacks.h"

class VkRenderer {
  public:
    VkRenderer(GLFWwindow *window);

    bool init(unsigned int width, unsigned int height);
    void setSize(unsigned int width, unsigned int height);

    bool draw(float deltaTime);

    void handleKeyEvents(int key, int scancode, int action, int mods);
    void handleMouseButtonEvents(int button, int action, int mods);
    void handleMousePositionEvents(double xPos, double yPos);
    void handleMouseWheelEvents(double xOffset, double yOffset);

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
    bool addLevel(std::string levelFileName, bool updateVertexData = true);
    void deleteLevel(std::string levelFileName);

    void addNullLevel();

    void requestExitApplication();
    void doExitApplication();

    ModelInstanceCamData& getModInstCamData();

    std::shared_ptr<BoundingBox3D> getWorldBoundaries();

    void cleanup();

  private:
    VkRenderData mRenderData{};
    ModelInstanceCamData mModelInstCamData{};

    Timer mFrameTimer{};
    Timer mMatrixGenerateTimer{};
    Timer mUploadToVBOTimer{};
    Timer mUploadToUBOTimer{};
    Timer mDownloadFromUBOTimer{};
    Timer mUIGenerateTimer{};
    Timer mUIDrawTimer{};
    Timer mCollisionDebugDrawTimer{};
    Timer mCollisionCheckTimer{};
    Timer mBehviorTimer{};
    Timer mInteractionTimer{};
    Timer mFaceAnimTimer{};
    Timer mLevelCollisionTimer{};
    Timer mIKTimer{};
    Timer mLevelGroundNeighborUpdateTimer{};
    Timer mPathFindingTimer{};

    UserInterface mUserInterface{};

    VkPushConstants mModelData{};
    VkComputePushConstants mComputeModelData{};
    VkUniformBufferData mPerspectiveViewMatrixUBO{};
    VkVertexBufferData mLineVertexBuffer{};
    VkVertexBufferData mSphereVertexBuffer{};
    VkVertexBufferData mLevelAABBVertexBuffer{};
    VkVertexBufferData mLevelOctreeVertexBuffer{};
    VkVertexBufferData mLevelWireframeVertexBuffer{};
    VkVertexBufferData mIKLinesVertexBuffer{};
    VkVertexBufferData mGroundMeshVertexBuffer{};
    VkVertexBufferData mGroundMeshNeighborVertexBuffer{};
    VkVertexBufferData mInstancePathVertexBuffer{};

    /* for animated and non-animated models */
    VkShaderStorageBufferData mShaderModelRootMatrixBuffer{};
    std::vector<glm::mat4> mWorldPosMatrices{};

    /* color hightlight for selection etc */
    std::vector<glm::vec2> mSelectedInstance{};
    VkShaderStorageBufferData mSelectedInstanceBuffer{};

    /* for animated models */
    VkShaderStorageBufferData mShaderBoneMatrixBuffer{};
    std::vector<PerInstanceAnimData> mPerInstanceAnimData{};
    VkShaderStorageBufferData mPerInstanceAnimDataBuffer{};
    std::vector<glm::mat4> mShaderBoneMatrices{};

    std::vector<AABB> mPerInstanceAABB{};
    std::shared_ptr<VkLineMesh> mAABBMesh = nullptr;

    /* for compute shader */
    bool mHasDedicatedComputeQueue = false;
    VkShaderStorageBufferData mShaderTRSMatrixBuffer{};

    /* bounding sphere compute shader */
    VkShaderStorageBufferData mSphereModelRootMatrixBuffer{};
    std::vector<glm::mat4> mSphereWorldPosMatrices{};
    std::vector<PerInstanceAnimData> mSpherePerInstanceAnimData{};
    VkShaderStorageBufferData mSpherePerInstanceAnimDataBuffer{};
    VkShaderStorageBufferData mSphereTRSMatrixBuffer{};
    VkShaderStorageBufferData mSphereBoneMatrixBuffer{};

    /* x/y/z is shpere center, w is radius */
    VkShaderStorageBufferData mBoundingSphereBuffer{};

    CoordArrowsModel mCoordArrowsModel{};
    RotationArrowsModel mRotationArrowsModel{};
    ScaleArrowsModel mScaleArrowsModel{};

    VkLineMesh mCoordArrowsMesh{};
    std::shared_ptr<VkLineMesh> mLineMesh = nullptr;

    SphereModel mSphereModel{};
    SphereModel mCollidingSphereModel{};
    VkLineMesh mSphereMesh{};
    VkLineMesh mCollidingSphereMesh{};

    unsigned int mLineIndexCount = 0;
    unsigned int mCollidingSphereCount = 0;

    bool mMouseLock = false;
    int mMouseXPos = 0;
    int mMouseYPos = 0;
    float mMouseWheelScale = 1.0f;
    int mMouseWheelScaleShiftKey = 0;
    bool mMouseWheelScrolling = false;
    std::chrono::time_point<std::chrono::steady_clock> mMouseWheelLastScrollTime{};
    CameraSettings mSavedCameraWheelSettings{};

    bool mMousePick = false;
    int mSavedSelectedInstanceId = 0;

    bool mMouseMove = false;
    bool mMouseMoveVertical = false;
    int mMouseMoveVerticalShiftKey = 0;
    InstanceSettings mSavedInstanceSettings{};

    void handleMovementKeys();
    void updateTriangleCount();
    void updateLevelTriangleCount();
    void enumerateInstances();

    /* identity matrices for view and perspective, zero matrix for light and fog  */
    VkUploadMatrices mMatrices{ glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(0.0f) };

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
    CameraSettings mSavedCameraSettings{};

    std::string generateUniqueCameraName(std::string camBaseName);
    bool checkCameraNameUsed(std::string cameraName);

    std::vector<glm::vec3> getPositionOfAllInstances();

    void initOctree(int thresholdPerBox, int maxDepth );
    std::shared_ptr<Octree> mOctree = nullptr;
    std::shared_ptr<BoundingBox3D> mWorldBoundaries = nullptr;

    bool createAABBLookup(std::shared_ptr<AssimpModel> model);
    void drawAABBs(std::vector<std::shared_ptr<AssimpInstance>> instances, glm::vec4 aabbColor);
    void drawCollisionDebug();
    bool createSelectedBoundingSpheres();
    bool createCollidingBoundingSpheres();
    bool createAllBoundingSpheres();
    std::map<int, std::vector<glm::vec4>> mBoundingSpheresPerInstance{};

    bool checkForInstanceCollisions();
    void checkForBorderCollisions();
    void checkForBoundingSphereCollisions();
    void reactToInstanceCollisions();

    void resetCollisionData();

    void findInteractionInstances();
    void drawInteractionDebug();

    std::shared_ptr<GraphEditor> mGraphEditor = nullptr;
    void editGraph(std::string graphName);
    std::shared_ptr<SingleInstanceBehavior> createEmptyGraph();

    std::shared_ptr<Behavior> mBehavior = nullptr;
    instanceNodeActionCallback mInstanceNodeActionCallbackFunction;

    std::vector<glm::vec4> mFaceAnimPerInstanceData{};
    VkShaderStorageBufferData mFaceAnimPerInstanceDataBuffer{};

    VkShaderStorageBufferData mShaderLevelRootMatrixBuffer{};
    std::vector<glm::mat4> mLevelWorldPosMatrices{};

    void generateLevelVertexData();
    void generateLevelAABB();
    void generateLevelOctree();
    void generateLevelWireframe();

    void resetLevelData();
    void initTriangleOctree(int thresholdPerBox, int maxDepth);
    std::shared_ptr<TriangleOctree> mTriangleOctree = nullptr;

    void checkForLevelCollisions();

    AABB mAllLevelAABB{};
    std::shared_ptr<VkLineMesh> mLevelAABBMesh = nullptr;
    std::shared_ptr<VkLineMesh> mLevelOctreeMesh = nullptr;
    std::shared_ptr<VkLineMesh> mLevelWireframeMesh = nullptr;
    std::shared_ptr<VkLineMesh> mLevelCollidingTriangleMesh = nullptr;

    IKSolver mIKSolver{};
    std::shared_ptr<VkLineMesh> mIKFootPointMesh = nullptr;
    std::array<std::vector<glm::vec3>, 2> mNewNodePositions{};
    std::vector<glm::mat4> mIKWorldPositionsToSolve{};
    std::vector<glm::vec3> mIKSolvedPositions{};
    std::vector<TRSMatrixData> mTRSData{};

    std::vector<glm::mat4> mIKMatrices{};
    VkShaderStorageBufferData mIKBoneMatrixBuffer{};
    VkShaderStorageBufferData mIKTRSMatrixBuffer{};

    PathFinder mPathFinder{};
    void generateGroundTriangleData();
    std::shared_ptr<VkLineMesh> mLevelGroundNeighborsMesh = nullptr;
    std::shared_ptr<VkLineMesh> mInstancePathMesh = nullptr;

    uint32_t mGroundMeshVertexCount = 0;

    std::vector<int> getNavTargets();
    std::random_device mRandomDevice{};
    std::default_random_engine mRandomEngine{};

    VkTextureData mSkyboxTexture{};
    SkyboxModel mSkyboxModel{};
    VkVertexBufferData mSkyboxBuffer{};

    /* Vulkan specific code */
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VmaAllocator rdAllocator = nullptr;

    VkDeviceSize mMinSSBOOffsetAlignment = 0;

    bool deviceInit();
    bool getQueues();
    bool initVma();
    bool createDescriptorPool();
    bool createDescriptorLayouts();
    bool createDescriptorSets();
    bool createDepthBuffer();
    bool createSelectionImage();
    bool createVertexBuffers();
    bool createMatrixUBO();
    bool createSSBOs();
    bool createSwapchain();
    bool createRenderPass();
    bool createPipelineLayouts();
    bool createPipelines();
    bool createFramebuffer();
    bool createCommandPools();
    bool createCommandBuffers();
    bool createSyncObjects();

    bool initUserInterface();

    bool recreateSwapchain();

    void updateDescriptorSets();
    void updateComputeDescriptorSets();
    void updateLevelDescriptorSets();
    void updateSphereComputeDescriptorSets();
    void updateIKComputeDescriptorSets();

    void runComputeShaders(std::shared_ptr<AssimpModel> model, int numInstances,
      uint32_t modelOffset, uint32_t instanceOffset, bool useEmtpyBoneOffsets = false);

    void runBoundingSphereComputeShaders(std::shared_ptr<AssimpModel> model, int numInstances,
      uint32_t modelOffset);

    bool runIKComputeShaders(std::shared_ptr<AssimpModel> model, int numInstances,
      uint32_t modelOffset, int totalNumberOfBones);

};
