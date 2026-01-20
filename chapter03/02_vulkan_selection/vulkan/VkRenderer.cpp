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

#include "Logger.h"

VkRenderer::VkRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;
}

bool VkRenderer::init(unsigned int width, unsigned int height) {
  /* randomize rand() */
  std::srand(static_cast<int>(time(nullptr)));

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

  if (!createLineVertexBuffer()) {
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

  // register callbacks
  mModelInstData.miModelCheckCallbackFunction = [this](std::string fileName) { return hasModel(fileName); };
  mModelInstData.miModelAddCallbackFunction = [this](std::string fileName) { return addModel(fileName); };
  mModelInstData.miModelDeleteCallbackFunction = [this](std::string modelName) { deleteModel(modelName); };

  mModelInstData.miInstanceAddCallbackFunction = [this](std::shared_ptr<AssimpModel> model) { return addInstance(model); };
  mModelInstData.miInstanceAddManyCallbackFunction = [this](std::shared_ptr<AssimpModel> model, int numInstances) { addInstances(model, numInstances); };
  mModelInstData.miInstanceDeleteCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { deleteInstance(instance) ;};
  mModelInstData.miInstanceCloneCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { cloneInstance(instance); };
  mModelInstData.miInstanceCloneManyCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance, int numClones) { cloneInstances(instance, numClones); };

  mModelInstData.miInstanceCenterCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { centerInstance(instance); };

  /* valid, but emtpy */
  mLineMesh = std::make_shared<VkLineMesh>();
  Logger::log(1, "%s: line mesh storage initialized\n", __FUNCTION__);

  /* create an empty null model and an instance from it */
  std::shared_ptr<AssimpModel> nullModel = std::make_shared<AssimpModel>();
  mModelInstData.miModelList.emplace_back(nullModel);
  std::shared_ptr<AssimpInstance> nullInstance = std::make_shared<AssimpInstance>(nullModel);
  mModelInstData.miAssimpInstancesPerModel[nullModel->getModelFileName()].emplace_back(nullInstance);
  mModelInstData.miAssimpInstances.emplace_back(nullInstance);
  assignInstanceIndices();

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

  mFrameTimer.start();

  Logger::log(1, "%s: Vulkan renderer initialized to %ix%i\n", __FUNCTION__, width, height);
  return true;
}

bool VkRenderer::deviceInit() {
  /* instance and window - we need at least Vukan 1.1 for the "VK_KHR_maintenance1" extension */
  vkb::InstanceBuilder instBuild;
  auto instRet = instBuild
  .use_default_debug_messenger()
  .request_validation_layers()
  .enable_extension(VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME) // required to use VK_EXT_swapchain_maintenance1
  .enable_extension(VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME) // required to use VK_EXT_surface_maintenance1
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

  /* We need VK_EXT_swapchain_maintenance1 for a present fence */
  VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1{};
  swapchainMaintenance1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT;
  swapchainMaintenance1.swapchainMaintenance1 = VK_TRUE;

  /* force anisotropy and line width */
  VkPhysicalDeviceFeatures vk10features{};
  vk10features.samplerAnisotropy = VK_TRUE;
  vk10features.wideLines = VK_TRUE;

  /* just get the first available device */
  vkb::PhysicalDeviceSelector physicalDevSel{mRenderData.rdVkbInstance};
  auto physicalDevSelRet = physicalDevSel
  .set_surface(mSurface)
  .set_required_features(vk10features)
  .add_required_extension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)
  .add_required_extension_features(swapchainMaintenance1)
  .select();

  if (!physicalDevSelRet) {
    Logger::log(1, "%s error: could not get physical devices\n", __FUNCTION__);
    return false;
  }

  mRenderData.rdVkbPhysicalDevice = physicalDevSelRet.value();
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
    /* compute transformation shader */
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
      Logger::log(1, "%s error: could not create Assimp transform compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
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

    std::vector<VkDescriptorSetLayoutBinding> assimpMatMultPerModelBindings =
      { assimpParentMatrixSsboBind, assimpBoneOffsetSsboBind};

    VkDescriptorSetLayoutCreateInfo assimpMatrixMultPerModelCreateInfo{};
    assimpMatrixMultPerModelCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    assimpMatrixMultPerModelCreateInfo.bindingCount = static_cast<uint32_t>(assimpMatMultPerModelBindings.size());
    assimpMatrixMultPerModelCreateInfo.pBindings = assimpMatMultPerModelBindings.data();

    result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpMatrixMultPerModelCreateInfo,
      nullptr, &mRenderData.rdAssimpComputeMatrixMultPerModelDescriptorLayout);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create Assimp matrix multiplication per model compute buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
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
    /* compute transform */
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

  updateDescriptorSets();
  updateComputeDescriptorSets();

  return true;
}

void VkRenderer::updateDescriptorSets() {
  Logger::log(1, "%s: updating descriptor sets\n", __FUNCTION__);
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
  Logger::log(1, "%s: updating compute descriptor sets\n", __FUNCTION__);
  {
    /* transform compute shader */
    VkDescriptorBufferInfo transformInfo{};
    transformInfo.buffer = mShaderNodeTransformBuffer.buffer;
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

  /* set surface to non-sRGB */
  surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;

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
  mRenderData.rdSwapchainImages = swapChainBuildRet.value().get_images().value();
  mRenderData.rdSwapchainImageViews = swapChainBuildRet.value().get_image_views().value();

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

bool VkRenderer::createLineVertexBuffer() {
  return VertexBuffer::init(mRenderData, mLineVertexBuffer, 1024);
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

  if (!ShaderStorageBuffer::init(mRenderData, mShaderNodeTransformBuffer)) {
    Logger::log(1, "%s error: could not create node transform SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mShaderBoneMatrixBuffer)) {
    Logger::log(1, "%s error: could not create bone matrix SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mSelectedInstanceBuffer)) {
    Logger::log(1, "%s error: could not create selection SSBO\n", __FUNCTION__);
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

  /* transform compute */
  std::vector<VkPushConstantRange> computePushConstants = { { VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(VkComputePushConstants) } };

  std::vector<VkDescriptorSetLayout> transformLayouts = {
    mRenderData.rdAssimpComputeTransformDescriptorLayout };

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

  /* line drawing */
  std::vector<VkDescriptorSetLayout> lineLayouts = {
    mRenderData.rdLineDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdLinePipelineLayout, lineLayouts)) {
    Logger::log(1, "%s error: could not init Assimp line drawing pipeline layout\n", __FUNCTION__);
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

  std::string computeShaderFile = "shader/assimp_instance_transform.comp.spv";
  if (!ComputePipeline::init(mRenderData, mRenderData.rdAssimpComputeTransformaPipelineLayout,
      mRenderData.rdAssimpComputeTransformPipeline, computeShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Transform compute shader pipeline\n", __FUNCTION__);
    return false;
  }

  computeShaderFile = "shader/assimp_instance_matrix_mult.comp.spv";
  if (!ComputePipeline::init(mRenderData, mRenderData.rdAssimpComputeMatrixMultPipelineLayout,
      mRenderData.rdAssimpComputeMatrixMultPipeline, computeShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Matrix Mult compute shader pipeline\n", __FUNCTION__);
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
  auto modelIter =  std::find_if(mModelInstData.miModelList.begin(), mModelInstData.miModelList.end(),
    [modelFileName](const auto& model) {
      return model->getModelFileNamePath() == modelFileName || model->getModelFileName() == modelFileName;
    });
  return modelIter != mModelInstData.miModelList.end();
}

std::shared_ptr<AssimpModel> VkRenderer::getModel(std::string modelFileName) {
  auto modelIter =  std::find_if(mModelInstData.miModelList.begin(), mModelInstData.miModelList.end(),
    [modelFileName](const auto& model) {
      return model->getModelFileNamePath() == modelFileName || model->getModelFileName() == modelFileName;
    });
  if (modelIter != mModelInstData.miModelList.end()) {
    return *modelIter;
  }
  return nullptr;
}

bool VkRenderer::addModel(std::string modelFileName) {
  if (hasModel(modelFileName)) {
    Logger::log(1, "%s warning: model '%s' already existed, skipping\n", __FUNCTION__, modelFileName.c_str());
    return false;
  }

  std::shared_ptr<AssimpModel> model = std::make_shared<AssimpModel>();
  if (!model->loadModel(mRenderData, modelFileName)) {
    Logger::log(1, "%s error: could not load model file '%s'\n", __FUNCTION__, modelFileName.c_str());
    return false;
  }

  mModelInstData.miModelList.emplace_back(model);

  /* also add a new instance here to see the model */
  addInstance(model);

  if (mModelInstData.miAssimpInstances.size() == 2) {
    std::shared_ptr<AssimpInstance> firstInstance = mModelInstData.miAssimpInstances.at(1);
    centerInstance(firstInstance);
  }

  return true;
}

void VkRenderer::deleteModel(std::string modelFileName) {
  std::string shortModelFileName = std::filesystem::path(modelFileName).filename().generic_string();

  mModelInstData.miAssimpInstances.erase(
    std::remove_if(
      mModelInstData.miAssimpInstances.begin(),
        mModelInstData.miAssimpInstances.end(),
       [shortModelFileName](std::shared_ptr<AssimpInstance> instance) {
         return instance->getModel()->getModelFileName() == shortModelFileName;
      }
    ),
    mModelInstData.miAssimpInstances.end()
  );

  if (mModelInstData.miAssimpInstancesPerModel.count(shortModelFileName) > 0) {
    mModelInstData.miAssimpInstancesPerModel[shortModelFileName].clear();
    mModelInstData.miAssimpInstancesPerModel.erase(shortModelFileName);
  }

  /* add models to pending delete list */
  for (const auto& model : mModelInstData.miModelList) {
    if (model && (model->getTriangleCount() > 0)) {
      mModelInstData.miPendingDeleteAssimpModels.insert(model);
    }
  }

  mModelInstData.miModelList.erase(
    std::remove_if(
      mModelInstData.miModelList.begin(),
      mModelInstData.miModelList.end(),
      [modelFileName](std::shared_ptr<AssimpModel> model) {
        return model->getModelFileName() == modelFileName;
      }
    )
  );

  assignInstanceIndices();
  updateTriangleCount();
}

std::shared_ptr<AssimpInstance> VkRenderer::addInstance(std::shared_ptr<AssimpModel> model) {
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
  mModelInstData.miAssimpInstances.emplace_back(newInstance);
  mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);

  assignInstanceIndices();
  updateTriangleCount();

  return newInstance;
}

void VkRenderer::addInstances(std::shared_ptr<AssimpModel> model, int numInstances) {
  size_t animClipNum = model->getAnimClips().size();
  for (int i = 0; i < numInstances; ++i) {
    int xPos = std::rand() % 50 - 25;
    int zPos = std::rand() % 50 - 25;
    int rotation = std::rand() % 360 - 180;
    int clipNr = std::rand() % animClipNum;
    float animSpeed = (std::rand() % 50 + 75) / 100.0f;

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model, glm::vec3(xPos, 0.0f, zPos), glm::vec3(0.0f, rotation, 0.0f));
    if (animClipNum > 0) {
      InstanceSettings instSettings = newInstance->getInstanceSettings();
      instSettings.isAnimClipNr = clipNr;
      instSettings.isAnimSpeedFactor = animSpeed;
      newInstance->setInstanceSettings(instSettings);
    }

    mModelInstData.miAssimpInstances.emplace_back(newInstance);
    mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }

  assignInstanceIndices();
  updateTriangleCount();
}

void VkRenderer::deleteInstance(std::shared_ptr<AssimpInstance> instance) {
  std::shared_ptr<AssimpModel> currentModel = instance->getModel();
  std::string currentModelName = currentModel->getModelFileName();

  mModelInstData.miAssimpInstances.erase(
    std::remove_if(
      mModelInstData.miAssimpInstances.begin(),
      mModelInstData.miAssimpInstances.end(),
      [instance](std::shared_ptr<AssimpInstance> inst) {
        return inst == instance;
      }
    ));


  mModelInstData.miAssimpInstancesPerModel[currentModelName].erase(
    std::remove_if(
      mModelInstData.miAssimpInstancesPerModel[currentModelName].begin(),
      mModelInstData.miAssimpInstancesPerModel[currentModelName].end(),
      [instance](std::shared_ptr<AssimpInstance> inst) {
        return inst == instance;
      }
    ));

  assignInstanceIndices();
  updateTriangleCount();
}

void VkRenderer::cloneInstance(std::shared_ptr<AssimpInstance> instance) {
  std::shared_ptr<AssimpModel> currentModel = instance->getModel();
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(currentModel);
  InstanceSettings newInstanceSettings = instance->getInstanceSettings();

  /* slight offset to see new instance */
  newInstanceSettings.isWorldPosition += glm::vec3(1.0f, 0.0f, -1.0f);
  newInstance->setInstanceSettings(newInstanceSettings);

  mModelInstData.miAssimpInstances.emplace_back(newInstance);
  mModelInstData.miAssimpInstancesPerModel[currentModel->getModelFileName()].emplace_back(newInstance);

  assignInstanceIndices();
  updateTriangleCount();
}

/* keep scaling and axis flipping */
void VkRenderer::cloneInstances(std::shared_ptr<AssimpInstance> instance, int numClones) {
  std::shared_ptr<AssimpModel> model = instance->getModel();
  size_t animClipNum = model->getAnimClips().size();
  for (int i = 0; i < numClones; ++i) {
    int xPos = std::rand() % 50 - 25;
    int zPos = std::rand() % 50 - 25;
    int rotation = std::rand() % 360 - 180;

    int clipNr = std::rand() % animClipNum;
    float animSpeed = (std::rand() % 50 + 75) / 100.0f;

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
    InstanceSettings instSettings = instance->getInstanceSettings();
    instSettings.isWorldPosition = glm::vec3(xPos, 0.0f, zPos);
    instSettings.isWorldRotation = glm::vec3(0.0f, rotation, 0.0f);
    if (animClipNum > 0) {
      instSettings.isAnimClipNr = clipNr;
      instSettings.isAnimSpeedFactor = animSpeed;
    }

    newInstance->setInstanceSettings(instSettings);

    mModelInstData.miAssimpInstances.emplace_back(newInstance);
    mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }

  assignInstanceIndices();
  updateTriangleCount();
}

void VkRenderer::centerInstance(std::shared_ptr<AssimpInstance> instance) {
  InstanceSettings instSettings = instance->getInstanceSettings();
  mCamera.moveCameraTo(mRenderData, instSettings.isWorldPosition + glm::vec3(5.0f));
}


void VkRenderer::updateTriangleCount() {
  mRenderData.rdTriangleCount = 0;
  for (const auto& instance : mModelInstData.miAssimpInstances) {
    mRenderData.rdTriangleCount += instance->getModel()->getTriangleCount();
  }
}

void VkRenderer::assignInstanceIndices() {
  for (size_t i = 0; i < mModelInstData.miAssimpInstances.size(); ++i) {
    InstanceSettings instSettings = mModelInstData.miAssimpInstances.at(i)->getInstanceSettings();
    instSettings.isInstanceIndexPosition = i;
    mModelInstData.miAssimpInstances.at(i)->setInstanceSettings(instSettings);
  }
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

void VkRenderer::handleKeyEvents(int key, int scancode, int action, int mods) {
  /* instance edit modes */
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_1) == GLFW_PRESS) {
    mRenderData.rdInstanceEditMode = instanceEditMode::move;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_2) == GLFW_PRESS) {
    mRenderData.rdInstanceEditMode = instanceEditMode::rotate;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_3) == GLFW_PRESS) {
    mRenderData.rdInstanceEditMode = instanceEditMode::scale;
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

  if (glfwGetKey(mRenderData.rdWindow, mMouseMoveVerticalShiftKey) == GLFW_RELEASE) {
    mMouseMoveVerticalShiftKey = 0;
    mMouseMoveVertical = false;
  }
}

void VkRenderer::handleMouseButtonEvents(int button, int action, int mods) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  if (button >= 0 && button < ImGuiMouseButton_COUNT) {
    io.AddMouseButtonEvent(button, action == GLFW_PRESS);
  }

  /* hide from application if above ImGui window */
  if (io.WantCaptureMouse && io.WantCaptureMouseUnlessPopupClose) {
    return;
  }

  /* trigger selection when left button has been released */
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    mMousePick = true;
    mRenderData.rdInstanceEditMode = instanceEditMode::move;
  }

  /* move instance around with middle button pressed */
  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
    mMouseMove = true;
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_LEFT_SHIFT;
      mMouseMoveVertical = true;
    }
    if  (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
      mMouseMoveVerticalShiftKey = GLFW_KEY_RIGHT_SHIFT;
      mMouseMoveVertical = true;
    }
  }

  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
    mMouseMove = false;
  }

  /* move camera view while right button is hold   */
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    mMouseLock = true;
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
    mMouseLock = false;
  }

  if (mMouseLock) {
    glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    /* enable raw mode if possible */
    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(mRenderData.rdWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
  } else {
    glfwSetInputMode(mRenderData.rdWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  }
}

void VkRenderer::handleMousePositionEvents(double xPos, double yPos) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent((float)xPos, (float)yPos);

  /* hide from application if above ImGui window */
  if (io.WantCaptureMouse && io.WantCaptureMouseUnlessPopupClose) {
    return;
  }

  /* calculate relative movement from last position */
  int mouseMoveRelX = static_cast<int>(xPos) - mMouseXPos;
  int mouseMoveRelY = static_cast<int>(yPos) - mMouseYPos;

  if (mMouseLock) {
    mRenderData.rdViewAzimuth += mouseMoveRelX / 10.0;
    /* keep between 0 and 360 degree */
    if (mRenderData.rdViewAzimuth < 0.0) {
      mRenderData.rdViewAzimuth += 360.0;
    }
    if (mRenderData.rdViewAzimuth >= 360.0) {
      mRenderData.rdViewAzimuth -= 360.0;
    }

    mRenderData.rdViewElevation -= mouseMoveRelY / 10.0;
    /* keep between -89 and +89 degree */
    mRenderData.rdViewElevation = std::clamp(mRenderData.rdViewElevation, -89.0f, 89.0f);

  }
  if (mMouseMove) {
    if (mModelInstData.miSelectedInstance != 0) {
      InstanceSettings settings = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance)->getInstanceSettings();

      float mouseXScaled = mouseMoveRelX / 20.0f;
      float mouseYScaled = mouseMoveRelY / 20.0f;
      float sinAzimuth = std::sin(glm::radians(mRenderData.rdViewAzimuth));
      float cosAzimuth = std::cos(glm::radians(mRenderData.rdViewAzimuth));

      float modelDistance = glm::length(mRenderData.rdCameraWorldPosition - settings.isWorldPosition) / 50.0f;

      if (mMouseMoveVertical) {
        switch(mRenderData.rdInstanceEditMode) {
          case instanceEditMode::move:
            settings.isWorldPosition.y -= mouseYScaled * modelDistance;
            break;
          case instanceEditMode::rotate:
            settings.isWorldRotation.y -= mouseXScaled * 5.0f;
            /* keep between -180 and 180 degree */
            if (settings.isWorldRotation.y < -180.0f) {
              settings.isWorldRotation.y += 360.0f;
            }
            if (settings.isWorldRotation.y >= 180.0f) {
              settings.isWorldRotation.y -= 360.0f;
            }
            break;
          case instanceEditMode::scale:
            /* uniform scale, do nothing here  */
            break;
        }
      } else {
        switch(mRenderData.rdInstanceEditMode) {
          case instanceEditMode::move:
            settings.isWorldPosition.x += mouseXScaled * modelDistance * cosAzimuth - mouseYScaled * modelDistance * sinAzimuth;
            settings.isWorldPosition.z += mouseXScaled * modelDistance * sinAzimuth + mouseYScaled * modelDistance * cosAzimuth;
            break;
          case instanceEditMode::rotate:
            settings.isWorldRotation.z -= (mouseXScaled * cosAzimuth - mouseYScaled * sinAzimuth) * 5.0f;
            settings.isWorldRotation.x += (mouseXScaled * sinAzimuth + mouseYScaled * cosAzimuth) * 5.0f;

            /* keep between -180 and 180 degree */
            if (settings.isWorldRotation.z < -180.0f) {
              settings.isWorldRotation.z += 360.0f;
            }
            if (settings.isWorldRotation.z >= 180.0f) {
              settings.isWorldRotation.z -= 360.0f;
            }

            if (settings.isWorldRotation.x < -180.0f) {
              settings.isWorldRotation.x += 360.0f;
            }
            if (settings.isWorldRotation.x >= 180.0f) {
              settings.isWorldRotation.x -= 360.0f;
            }
            break;
          case instanceEditMode::scale:
            settings.isScale -= mouseYScaled / 2.0f;
            settings.isScale = std::max(0.001f, settings.isScale);
            break;
        }
      }

      mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance)->setInstanceSettings(settings);
    }
  }

  /* save old values */
  mMouseXPos = static_cast<int>(xPos);
  mMouseYPos = static_cast<int>(yPos);
}

void VkRenderer::handleMovementKeys() {
  /* hide from application */
  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  mRenderData.rdMoveForward = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_W) == GLFW_PRESS) {
    mRenderData.rdMoveForward += 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_S) == GLFW_PRESS) {
    mRenderData.rdMoveForward -= 1;
  }

  mRenderData.rdMoveRight = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_A) == GLFW_PRESS) {
    mRenderData.rdMoveRight -= 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_D) == GLFW_PRESS) {
    mRenderData.rdMoveRight += 1;
  }

  mRenderData.rdMoveUp = 0;
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_E) == GLFW_PRESS) {
    mRenderData.rdMoveUp += 1;
  }
  if (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_Q) == GLFW_PRESS) {
    mRenderData.rdMoveUp -= 1;
  }

  /* speed up movement with shift */
  if ((glfwGetKey(mRenderData.rdWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
      (glfwGetKey(mRenderData.rdWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)) {
    mRenderData.rdMoveForward *= 10;
    mRenderData.rdMoveRight *= 10;
    mRenderData.rdMoveUp *= 10;
  }
}

void VkRenderer::runComputeShaders(std::shared_ptr<AssimpModel> model, int numInstances, uint32_t modelOffset) {
  uint32_t numberOfBones = static_cast<uint32_t>(model->getBoneList().size());

  /* node transformation */
  vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeTransformPipeline);
  vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeTransformaPipelineLayout, 0, 1, &mRenderData.rdAssimpComputeTransformDescriptorSet, 0, 0);

  mUploadToUBOTimer.start();
  mComputeModelData.pkModelOffset = modelOffset;
  vkCmdPushConstants(mRenderData.rdComputeCommandBuffer, mRenderData.rdAssimpComputeTransformaPipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(VkComputePushConstants)), &mComputeModelData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  vkCmdDispatch(mRenderData.rdComputeCommandBuffer, numberOfBones, static_cast<uint32_t>(std::ceil(numInstances / 32.0f)), 1);

  /* memroy barrier between the compute shaders
   * wait for TRS buffer to be written  */
  VkBufferMemoryBarrier trsBufferBarrier{};
  trsBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  trsBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  trsBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  trsBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  trsBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  trsBufferBarrier.buffer = mShaderTRSMatrixBuffer.buffer;
  trsBufferBarrier.offset = 0;
  trsBufferBarrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
    &trsBufferBarrier, 0, nullptr);

  /* matrix multiplication */
  vkCmdBindPipeline(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeMatrixMultPipeline);

  VkDescriptorSet &modelDescriptorSet = model->getMatrixMultDescriptorSet();
  std::vector<VkDescriptorSet> computeSets = { mRenderData.rdAssimpComputeMatrixMultDescriptorSet, modelDescriptorSet };
  vkCmdBindDescriptorSets(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
    mRenderData.rdAssimpComputeMatrixMultPipelineLayout, 0, static_cast<uint32_t>(computeSets.size()), computeSets.data(), 0, 0);

  mUploadToUBOTimer.start();
  mComputeModelData.pkModelOffset = modelOffset;
  vkCmdPushConstants(mRenderData.rdComputeCommandBuffer, mRenderData.rdAssimpComputeMatrixMultPipelineLayout,
    VK_SHADER_STAGE_COMPUTE_BIT, 0, static_cast<uint32_t>(sizeof(VkComputePushConstants)), &mComputeModelData);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  vkCmdDispatch(mRenderData.rdComputeCommandBuffer, numberOfBones, static_cast<uint32_t>(std::ceil(numInstances / 32.0f)), 1);

  /* memroy barrier after compute shader
   * wait for bone matrix buffer to be written  */
  VkBufferMemoryBarrier boneMatrixBufferBarrier{};
  boneMatrixBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  boneMatrixBufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  boneMatrixBufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  boneMatrixBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  boneMatrixBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  boneMatrixBufferBarrier.buffer = mShaderBoneMatrixBuffer.buffer;
  boneMatrixBufferBarrier.offset = 0;
  boneMatrixBufferBarrier.size = VK_WHOLE_SIZE;

  vkCmdPipelineBarrier(mRenderData.rdComputeCommandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
    &boneMatrixBufferBarrier, 0, nullptr);
}

bool VkRenderer::draw(float deltaTime) {
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

  /* wait for all fences before getting the new framebuffer image */
  std::vector<VkFence> waitFences = {
    mRenderData.rdComputeFence,
    mRenderData.rdPresentFence,
    mRenderData.rdRenderFence
  };

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

  /* calculate the size of the node matrix buffer over all animated instances */
  size_t boneMatrixBufferSize = 0;
  for (const auto& model : mModelInstData.miModelList) {
    size_t numberOfInstances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() && !model->getBoneList().empty()) {
        size_t numberOfBones = model->getBoneList().size();

        /* buffer size must always be a multiple of "local_size_y" instances to avoid undefined behavior */
        boneMatrixBufferSize += numberOfBones * ((numberOfInstances - 1) / 32 + 1) * 32;
      }
    }
  }


  /* clear and resize world pos matrices */
  mWorldPosMatrices.clear();
  mWorldPosMatrices.resize(mModelInstData.miAssimpInstances.size());
  mShaderNodeTransFormData.clear();
  mShaderNodeTransFormData.resize(boneMatrixBufferSize);
  mSelectedInstance.clear();
  mSelectedInstance.resize(mModelInstData.miAssimpInstances.size());

  /* save the selected instance for color highlight */
  std::shared_ptr<AssimpInstance> currentSelectedInstance = nullptr;
  if (mRenderData.rdHighlightSelectedInstance) {
    currentSelectedInstance = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance);
    mRenderData.rdSelectedInstanceHighlightValue += deltaTime * 4.0f;
    if (mRenderData.rdSelectedInstanceHighlightValue > 2.0f) {
      mRenderData.rdSelectedInstanceHighlightValue = 0.1f;
    }
  }

  /* we need to track the presence of animated models */
  bool animatedModelLoaded = false;

  size_t instanceToStore = 0;
  size_t animatedInstancesToStore = 0;
  for (const auto& model : mModelInstData.miModelList) {
    size_t numberOfInstances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() && !model->getBoneList().empty()) {
        size_t numberOfBones = model->getBoneList().size();
        animatedModelLoaded = true;

        mMatrixGenerateTimer.start();

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()];
        for (unsigned int i = 0; i < numberOfInstances; ++i) {
          instances.at(i)->updateAnimation(deltaTime);
          std::vector<NodeTransformData> instanceNodeTransform = instances.at(i)->getNodeTransformData();
          std::copy(instanceNodeTransform.begin(), instanceNodeTransform.end(), mShaderNodeTransFormData.begin() + animatedInstancesToStore + i * numberOfBones);
          mWorldPosMatrices.at(instanceToStore + i) = instances.at(i)->getWorldTransformMatrix();

          if (currentSelectedInstance == instances.at(i)) {
            mSelectedInstance.at(instanceToStore + i).x = mRenderData.rdSelectedInstanceHighlightValue;
          } else {
            mSelectedInstance.at(instanceToStore + i).x = 1.0f;
          }

          if (mMousePick) {
            InstanceSettings instSettings = instances.at(i)->getInstanceSettings();
            mSelectedInstance.at(instanceToStore + i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
          }
        }

        size_t trsMatrixSize = numberOfBones * numberOfInstances * sizeof(glm::mat4);

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
        mRenderData.rdMatricesSize += trsMatrixSize;

        instanceToStore += numberOfInstances;
        animatedInstancesToStore += numberOfInstances * numberOfBones;
      } else {
        /* non-animated models */
        mMatrixGenerateTimer.start();

        std::vector<std::shared_ptr<AssimpInstance>> instances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()];
        for (unsigned int i = 0; i < numberOfInstances; ++i) {
          mWorldPosMatrices.at(instanceToStore + i) = instances.at(i)->getWorldTransformMatrix();

          if (currentSelectedInstance == instances.at(i)) {
            mSelectedInstance.at(instanceToStore + i).x = mRenderData.rdSelectedInstanceHighlightValue;
          } else {
            mSelectedInstance.at(instanceToStore + i).x = 1.0f;
          }

          if (mMousePick) {
            InstanceSettings instSettings = instances.at(i)->getInstanceSettings();
            mSelectedInstance.at(instanceToStore + i).y = static_cast<float>(instSettings.isInstanceIndexPosition);
          }
        }

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
        mRenderData.rdMatricesSize += numberOfInstances * sizeof(glm::mat4);

        instanceToStore += numberOfInstances;
      }
    }
  }

  /* we need to update descriptors after the upload if buffer size changed */
  bool bufferResized = false;
  mUploadToUBOTimer.start();
  bufferResized = ShaderStorageBuffer::uploadSsboData(mRenderData, mShaderNodeTransformBuffer, mShaderNodeTransFormData);
  bufferResized |= ShaderStorageBuffer::uploadSsboData(mRenderData, mSelectedInstanceBuffer, mSelectedInstance);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  /* resize SSBO if needed */
  bufferResized |= ShaderStorageBuffer::checkForResize(mRenderData, mShaderTRSMatrixBuffer, boneMatrixBufferSize * sizeof(glm::mat4));
  bufferResized |= ShaderStorageBuffer::checkForResize(mRenderData, mShaderBoneMatrixBuffer, boneMatrixBufferSize * sizeof(glm::mat4));

  if (bufferResized) {
    updateDescriptorSets();
    updateComputeDescriptorSets();
  }

  /* record compute commands */
  result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdComputeFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: compute fence reset failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  if (animatedModelLoaded) {
    if (!CommandBuffer::reset(mRenderData.rdComputeCommandBuffer, 0)) {
      Logger::log(1, "%s error: failed to reset compute command buffer\n", __FUNCTION__);
      return false;
    }

    if (!CommandBuffer::beginSingleShot(mRenderData.rdComputeCommandBuffer)) {
      Logger::log(1, "%s error: failed to begin compute command buffer\n", __FUNCTION__);
      return false;
    }

    uint32_t computeShaderModelOffset = 0;
    for (const auto& model : mModelInstData.miModelList) {
      size_t numberOfInstances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].size();
      if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

        /* compute shader for animated models only */
        if (model->hasAnimations() && !model->getBoneList().empty()) {
          size_t numberOfBones = model->getBoneList().size();

          runComputeShaders(model, numberOfInstances, computeShaderModelOffset);

          computeShaderModelOffset += numberOfInstances * numberOfBones;
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
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores = &mRenderData.rdComputeSemaphore;
    computeSubmitInfo.waitSemaphoreCount = 1;
    computeSubmitInfo.pWaitSemaphores = &mRenderData.rdGraphicSemaphore;
    computeSubmitInfo.pWaitDstStageMask = &waitStage;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };
  } else {
    /* do an empty submit if we don't have animated models to satisfy fence and semaphor */
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores = &mRenderData.rdComputeSemaphore;
    computeSubmitInfo.waitSemaphoreCount = 1;
    computeSubmitInfo.pWaitSemaphores = &mRenderData.rdGraphicSemaphore;
    computeSubmitInfo.pWaitDstStageMask = &waitStage;

    result = vkQueueSubmit(mRenderData.rdComputeQueue, 1, &computeSubmitInfo, mRenderData.rdComputeFence);
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to submit compute command buffer (%i)\n", __FUNCTION__, result);
      return false;
    };
  }

  handleMovementKeys();

  mMatrixGenerateTimer.start();
  mCamera.updateCamera(mRenderData, deltaTime);

  mMatrices.projectionMatrix = glm::perspective(
    glm::radians(static_cast<float>(mRenderData.rdFieldOfView)),
    static_cast<float>(mRenderData.rdVkbSwapchain.extent.width) / static_cast<float>(mRenderData.rdVkbSwapchain.extent.height),
    0.1f, 500.0f);

  mMatrices.viewMatrix = mCamera.getViewMatrix(mRenderData);

  mRenderData.rdMatrixGenerateTime = mMatrixGenerateTimer.stop();

  /* we need to update descriptors after the upload if buffer size changed */
  mUploadToUBOTimer.start();
  UniformBuffer::uploadData(mRenderData, mPerspectiveViewMatrixUBO, mMatrices);
  bufferResized = ShaderStorageBuffer::uploadSsboData(mRenderData, mShaderModelRootMatrixBuffer, mWorldPosMatrices);
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  if (bufferResized) {
    updateDescriptorSets();
  }

  /* start with graphics rendering */
  std::vector<VkFence> resetFences = {
    mRenderData.rdRenderFence,
    mRenderData.rdPresentFence
  };

  result = vkResetFences(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(resetFences.size()), resetFences.data());
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

  uint32_t worldPosOffset = 0;
  uint32_t skinMatOffset = 0;
  for (const auto& model : mModelInstData.miModelList) {
    size_t numberOfInstances = mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].size();
    if (numberOfInstances > 0 && model->getTriangleCount() > 0) {

      /* animated models */
      if (model->hasAnimations() && !model->getBoneList().empty()) {
        size_t numberOfBones = model->getBoneList().size();

        if (mMousePick) {
          vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdAssimpSkinningSelectionPipeline);

          vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpSkinningSelectionPipelineLayout, 1, 1, &mRenderData.rdAssimpSkinningSelectionDescriptorSet, 0, nullptr);
        } else {
          vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdAssimpSkinningPipeline);

          vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
            mRenderData.rdAssimpSkinningPipelineLayout, 1, 1, &mRenderData.rdAssimpSkinningDescriptorSet, 0, nullptr);
        }

        mUploadToUBOTimer.start();
        mModelData.pkModelStride = numberOfBones;
        mModelData.pkWorldPosOffset = worldPosOffset;
        mModelData.pkSkinMatOffset = skinMatOffset;
        if (mMousePick) {
          vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSkinningSelectionPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
        } else {
          vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSkinningPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0, static_cast<uint32_t>(sizeof(VkPushConstants)), &mModelData);
        }
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        model->drawInstanced(mRenderData, numberOfInstances, mMousePick);

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
  mCoordArrowsLineIndexCount = 0;
  mLineMesh->vertices.clear();
  if (mModelInstData.miSelectedInstance > 0) {
    InstanceSettings instSettings = mModelInstData.miAssimpInstances.at(mModelInstData.miSelectedInstance)->getInstanceSettings();

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

    mCoordArrowsLineIndexCount += mCoordArrowsMesh.vertices.size();
    std::for_each(mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end(),
                  [=](auto &n) {
                    n.color /= 2.0f;
                    n.position = glm::quat(glm::radians(instSettings.isWorldRotation)) * n.position;
                    n.position += instSettings.isWorldPosition;
                  });
    mLineMesh->vertices.insert(mLineMesh->vertices.end(),
                               mCoordArrowsMesh.vertices.begin(), mCoordArrowsMesh.vertices.end());
  }

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

  if (mCoordArrowsLineIndexCount > 0) {
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

  vkCmdEndRenderPass(mRenderData.rdLineCommandBuffer);

  if (!CommandBuffer::end(mRenderData.rdLineCommandBuffer)) {
    Logger::log(1, "%s error: failed to end line drawing command buffer\n", __FUNCTION__);
    return false;
  }

  /* imGui overlay */
  mUIGenerateTimer.start();
  mUserInterface.hideMouse(mMouseLock);
  mUserInterface.createFrame(mRenderData, mModelInstData);
  mRenderData.rdUIGenerateTime += mUIGenerateTimer.stop();

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
  mRenderData.rdUIDrawTime = mUIDrawTimer.stop();

  vkCmdEndRenderPass(mRenderData.rdImGuiCommandBuffer);

  if (!CommandBuffer::end(mRenderData.rdImGuiCommandBuffer)) {
    Logger::log(1, "%s error: failed to end ImGui command buffer\n", __FUNCTION__);
    return false;
  }

  /* submit command buffer */
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  std::vector<VkSemaphore> waitSemaphores = { mRenderData.rdComputeSemaphore, mRenderData.rdPresentSemaphore };
  std::vector<VkPipelineStageFlags> waitStages = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

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
  if (mMousePick) {
    /* wait for queue to be idle */
    vkQueueWaitIdle(mRenderData.rdGraphicsQueue);

    float selectedInstanceId = SelectionFramebuffer::getPixelValueFromPos(mRenderData, mMouseXPos, mMouseYPos);

    if (selectedInstanceId >= 0.0f) {
      mModelInstData.miSelectedInstance = static_cast<int>(selectedInstanceId);
    } else {
      mModelInstData.miSelectedInstance = 0;
    }
    mMousePick = false;
  }

  VkSwapchainPresentFenceInfoEXT presentFenceInfo{};
  presentFenceInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT;
  presentFenceInfo.swapchainCount = 1;
  presentFenceInfo.pFences = &mRenderData.rdPresentFence;

  /* trigger swapchain image presentation */
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = &presentFenceInfo;

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
  for (const auto& model : mModelInstData.miModelList) {
    model->cleanup(mRenderData);
  }

  for (const auto& model : mModelInstData.miPendingDeleteAssimpModels) {
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

  Framebuffer::cleanup(mRenderData);
  SelectionFramebuffer::cleanup(mRenderData);

  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSkinningPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSelectionPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSkinningSelectionPipeline);
  LinePipeline::cleanup(mRenderData, mRenderData.rdLinePipeline);

  ComputePipeline::cleanup(mRenderData, mRenderData.rdAssimpComputeTransformPipeline);
  ComputePipeline::cleanup(mRenderData, mRenderData.rdAssimpComputeMatrixMultPipeline);

  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSkinningPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpComputeTransformaPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpComputeMatrixMultPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSelectionPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSkinningSelectionPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdLinePipelineLayout);

  Renderpass::cleanup(mRenderData, mRenderData.rdRenderpass);
  SecondaryRenderpass::cleanup(mRenderData, mRenderData.rdImGuiRenderpass);
  SecondaryRenderpass::cleanup(mRenderData, mRenderData.rdLineRenderpass);
  SelectionRenderpass::cleanup(mRenderData);

  UniformBuffer::cleanup(mRenderData, mPerspectiveViewMatrixUBO);
  ShaderStorageBuffer::cleanup(mRenderData, mShaderTRSMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mShaderNodeTransformBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mShaderModelRootMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mShaderBoneMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mSelectedInstanceBuffer);

  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpSkinningDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpComputeTransformDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpComputeMatrixMultDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpSelectionDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpSkinningSelectionDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdLineDescriptorSet);

  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSkinningDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpTextureDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeTransformDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeMatrixMultDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpComputeMatrixMultPerModelDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSelectionDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpSkinningSelectionDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdLineDescriptorLayout, nullptr);

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
