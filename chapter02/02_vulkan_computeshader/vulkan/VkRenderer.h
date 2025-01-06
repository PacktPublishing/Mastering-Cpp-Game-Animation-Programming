/* Vulkan renderer */
#pragma once

#include <vector>
#include <string>
#include <memory>

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
#include "Camera.h"

#include "VkRenderData.h"
#include "ModelAndInstanceData.h"

class VkRenderer {
  public:
    VkRenderer(GLFWwindow *window);

    bool init(unsigned int width, unsigned int height);
    void setSize(unsigned int width, unsigned int height);

    bool draw(float deltaTime);

    void handleKeyEvents(int key, int scancode, int action, int mods);
    void handleMouseButtonEvents(int button, int action, int mods);
    void handleMousePositionEvents(double xPos, double yPos);

    bool hasModel(std::string modelFileName);
    std::shared_ptr<AssimpModel> getModel(std::string modelFileName);
    bool addModel(std::string modelFileName);
    void deleteModel(std::string modelFileName);

    std::shared_ptr<AssimpInstance> addInstance(std::shared_ptr<AssimpModel> model);
    void addInstances(std::shared_ptr<AssimpModel> model, int numInstances);
    void deleteInstance(std::shared_ptr<AssimpInstance> instance);
    void cloneInstance(std::shared_ptr<AssimpInstance> instance);

    void cleanup();

  private:
    VkRenderData mRenderData{};
    ModelAndInstanceData mModelInstData{};

    Timer mFrameTimer{};
    Timer mMatrixGenerateTimer{};
    Timer mUploadToUBOTimer{};
    Timer mUIGenerateTimer{};
    Timer mUIDrawTimer{};

    UserInterface mUserInterface{};
    Camera mCamera{};

    VkPushConstants mModelData{};
    VkComputePushConstants mComputeModelData{};
    VkUniformBufferData mPerspectiveViewMatrixUBO{};

    /* for animated and non-animated models */
    VkShaderStorageBufferData mShaderModelRootMatrixBuffer{};
    std::vector<glm::mat4> mWorldPosMatrices{};

    /* for animated models */
    VkShaderStorageBufferData mShaderBoneMatrixBuffer{};

    /* for compute shader */
    bool mHasDedicatedComputeQueue = false;
    VkShaderStorageBufferData mShaderTRSMatrixBuffer{};
    VkShaderStorageBufferData mShaderNodeTransformBuffer{};
    std::vector<NodeTransformData> mNodeTransFormData{};

    bool mMouseLock = false;
    int mMouseXPos = 0;
    int mMouseYPos = 0;

    void handleMovementKeys();
    void updateTriangleCount();

    /* identity matrices */
    VkUploadMatrices mMatrices{ glm::mat4(1.0f), glm::mat4(1.0f) };

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

    void runComputeShaders(std::shared_ptr<AssimpModel> model, int numInstances, uint32_t modelOffset);
};
