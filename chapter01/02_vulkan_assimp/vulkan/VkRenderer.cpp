#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <memory>

#include <imgui_impl_glfw.h>

#include <glm/gtc/matrix_transform.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "VkRenderer.h"

#include "Framebuffer.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "SyncObjects.h"
#include "Renderpass.h"

#include "PipelineLayout.h"
#include "SkinningPipeline.h"

#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "InstanceSettings.h"

#include "Logger.h"

VkRenderer::VkRenderer(GLFWwindow *window) {
  mRenderData.rdWindow = window;
}

bool VkRenderer::init(unsigned int width, unsigned int height) {
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

  if (!createCommandPool()) {
    return false;
  }

  if (!createCommandBuffer()) {
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

  if (!createAssimpPipeline()) {
    return false;
  }

  if (!createAssimpSkinningPipeline()) {
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

  mWorldPosMatrices.resize(1);
  mWorldPosMatrices.at(0) = glm::mat4(1.0f);

  // register callbacks
  mModelInstData.miModelCheckCallbackFunction = [this](std::string fileName) { return hasModel(fileName); };
  mModelInstData.miModelAddCallbackFunction = [this](std::string fileName) { return addModel(fileName); };
  mModelInstData.miModelDeleteCallbackFunction = [this](std::string modelName) { deleteModel(modelName); };

  mModelInstData.miInstanceAddCallbackFunction = [this](std::shared_ptr<AssimpModel> model) { return addInstance(model); };
  mModelInstData.miInstanceAddManyCallbackFunction = [this](std::shared_ptr<AssimpModel> model, int numInstances) { addInstances(model, numInstances); };
  mModelInstData.miInstanceDeleteCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { deleteInstance(instance) ;};
  mModelInstData.miInstanceCloneCallbackFunction = [this](std::shared_ptr<AssimpInstance> instance) { cloneInstance(instance); };

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

  VkResult result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpTextureCreateInfo,
    nullptr, &mRenderData.rdAssimpTextureDescriptorLayout);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create ASSIMP texturedescriptor set layout (error: %i)\n", __FUNCTION__, result);
    return false;
  }

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

  std::vector<VkDescriptorSetLayoutBinding> assimpBindings = { assimpUboBind, assimpSsboBind };

  VkDescriptorSetLayoutCreateInfo assimpCreateInfo{};
  assimpCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  assimpCreateInfo.bindingCount = static_cast<uint32_t>(assimpBindings.size());
  assimpCreateInfo.pBindings = assimpBindings.data();

  result = vkCreateDescriptorSetLayout(mRenderData.rdVkbDevice.device, &assimpCreateInfo,
    nullptr, &mRenderData.rdAssimpDescriptorLayout);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create ASSIMP buffer descriptor set layout (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  return true;
}

bool VkRenderer::createDescriptorSets() {
  /* non-animated models */
  VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
  descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
  descriptorAllocateInfo.descriptorSetCount = 1;
  descriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpDescriptorLayout;

  VkResult result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &descriptorAllocateInfo,
      &mRenderData.rdAssimpDescriptorSet);
   if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate Assimp SSBO descriptor set (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  /* animated models */
  VkDescriptorSetAllocateInfo skinningDescriptorAllocateInfo{};
  skinningDescriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  skinningDescriptorAllocateInfo.descriptorPool = mRenderData.rdDescriptorPool;
  skinningDescriptorAllocateInfo.descriptorSetCount = 1;
  skinningDescriptorAllocateInfo.pSetLayouts = &mRenderData.rdAssimpDescriptorLayout;

  result = vkAllocateDescriptorSets(mRenderData.rdVkbDevice.device, &skinningDescriptorAllocateInfo,
    &mRenderData.rdAssimpSkinningDescriptorSet);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate Assimp Skinning SSBO descriptor set (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  updateDescriptorSets();

  return true;
}

bool VkRenderer::updateDescriptorSets() {
  /* we must update the descriptor sets whenever the buffer size has changed */
  VkDescriptorBufferInfo matrixInfo{};
  matrixInfo.buffer = mPerspectiveViewMatrixUBO.buffer;
  matrixInfo.offset = 0;
  matrixInfo.range = VK_WHOLE_SIZE;

  VkDescriptorBufferInfo worldPosInfo{};
  worldPosInfo.buffer = mWorldPosBuffer.buffer;
  worldPosInfo.offset = 0;
  worldPosInfo.range = VK_WHOLE_SIZE;

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

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = { matrixWriteDescriptorSet, posWriteDescriptorSet };

  vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(writeDescriptorSets.size()),
     writeDescriptorSets.data(), 0, nullptr);

  VkDescriptorBufferInfo boneMatrixInfo{};
  boneMatrixInfo.buffer = mBoneMatrixBuffer.buffer;
  boneMatrixInfo.offset = 0;
  boneMatrixInfo.range = VK_WHOLE_SIZE;

  /* world pos matrix is identical, just needs another descriptor set */
  matrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningDescriptorSet;

  VkWriteDescriptorSet boneMatrixWriteDescriptorSet{};
  boneMatrixWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  boneMatrixWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  boneMatrixWriteDescriptorSet.dstSet = mRenderData.rdAssimpSkinningDescriptorSet;
  boneMatrixWriteDescriptorSet.dstBinding = 1;
  boneMatrixWriteDescriptorSet.descriptorCount = 1;
  boneMatrixWriteDescriptorSet.pBufferInfo = &boneMatrixInfo;

  std::vector<VkWriteDescriptorSet> skinningWriteDescriptorSets = { matrixWriteDescriptorSet, boneMatrixWriteDescriptorSet };

  vkUpdateDescriptorSets(mRenderData.rdVkbDevice.device, static_cast<uint32_t>(skinningWriteDescriptorSets.size()),
     skinningWriteDescriptorSets.data(), 0, nullptr);
  return true;
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
    Logger::log(1, "%s error: could not create depth buffer image view (error: %i)\n", __FUNCTION__), result;
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
  while (mRenderData.rdWidth == 0 || mRenderData.rdHeight == 0) {
    glfwGetFramebufferSize(mRenderData.rdWindow, &mRenderData.rdWidth, &mRenderData.rdHeight);
    glfwWaitEvents();
  }
  vkDeviceWaitIdle(mRenderData.rdVkbDevice.device);

  /* cleanup */
  Framebuffer::cleanup(mRenderData);
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

  if (!createFramebuffer()) {
    Logger::log(1, "%s error: could not recreate framebuffers\n", __FUNCTION__);
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
  if (!ShaderStorageBuffer::init(mRenderData, mWorldPosBuffer)) {
    Logger::log(1, "%s error: could not create world position SSBO\n", __FUNCTION__);
    return false;
  }

  if (!ShaderStorageBuffer::init(mRenderData, mBoneMatrixBuffer)) {
    Logger::log(1, "%s error: could not create bone matrix SSBO\n", __FUNCTION__);
    return false;
  }

  return true;
}


bool VkRenderer::createRenderPass() {
  if (!Renderpass::init(mRenderData)) {
    Logger::log(1, "%s error: could not init renderpass\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createPipelineLayouts() {
  /* non-animated model */
  std::vector<VkDescriptorSetLayout> layouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpDescriptorLayout };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpPipelineLayout, layouts)) {
    Logger::log(1, "%s error: could not init Assimp pipeline layout\n", __FUNCTION__);
    return false;
  }

  /* animated model, needs push constant */
  std::vector<VkDescriptorSetLayout> skinningLayouts = {
    mRenderData.rdAssimpTextureDescriptorLayout,
    mRenderData.rdAssimpDescriptorLayout };

  std::vector<VkPushConstantRange> pushConstants = { { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkPushConstants) } };

  if (!PipelineLayout::init(mRenderData, mRenderData.rdAssimpSkinningPipelineLayout, skinningLayouts, pushConstants)) {
    Logger::log(1, "%s error: could not init Assimp Skinning pipeline layout\n", __FUNCTION__);
    return false;
  }

  return true;
}

bool VkRenderer::createAssimpPipeline() {
  std::string vertexShaderFile = "shader/assimp.vert.spv";
  std::string fragmentShaderFile = "shader/assimp.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpPipelineLayout,
      mRenderData.rdAssimpPipeline, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp shader pipeline\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createAssimpSkinningPipeline() {
  std::string vertexShaderFile = "shader/assimp_skinning.vert.spv";
  std::string fragmentShaderFile = "shader/assimp_skinning.frag.spv";
  if (!SkinningPipeline::init(mRenderData, mRenderData.rdAssimpSkinningPipelineLayout,
      mRenderData.rdAssimpSkinningPipeline, vertexShaderFile, fragmentShaderFile)) {
    Logger::log(1, "%s error: could not init Assimp Skinning shader pipeline\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createFramebuffer() {
  if (!Framebuffer::init(mRenderData)) {
    Logger::log(1, "%s error: could not init framebuffer\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createCommandPool() {
  if (!CommandPool::init(mRenderData)) {
    Logger::log(1, "%s error: could not create command pool\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool VkRenderer::createCommandBuffer() {
  if (!CommandBuffer::init(mRenderData, mRenderData.rdCommandBuffer)) {
    Logger::log(1, "%s error: could not create command buffers\n", __FUNCTION__);
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

  /* also add a new instance here to see the model*/
  addInstance(model);

  return true;
}

void VkRenderer::deleteModel(std::string modelFileName) {
  std::string shortModelFileName = std::filesystem::path(modelFileName).filename().generic_string();

  if (mModelInstData.miAssimpInstances.size() > 0) {
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
  }

  if (mModelInstData.miAssimpInstancesPerModel.count(shortModelFileName) > 0) {
    mModelInstData.miAssimpInstancesPerModel[shortModelFileName].clear();
    mModelInstData.miAssimpInstancesPerModel.erase(shortModelFileName);
  }

  /* save model in separate pending deletion list before purging from model list */
  std::shared_ptr<AssimpModel> model = getModel(shortModelFileName);
  if (model) {
    mModelInstData.miPendingDeleteAssimpModels.emplace_back(model);
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

  updateTriangleCount();
}

std::shared_ptr<AssimpInstance> VkRenderer::addInstance(std::shared_ptr<AssimpModel> model) {
  std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model);
  mModelInstData.miAssimpInstances.emplace_back(newInstance);
  mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);

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

    std::shared_ptr<AssimpInstance> newInstance = std::make_shared<AssimpInstance>(model, glm::vec3(xPos, 0.0f, zPos), glm::vec3(0.0f, rotation, 0.0f));
    if (animClipNum > 0) {
      InstanceSettings instSettings = newInstance->getInstanceSettings();
      instSettings.isAnimClipNr = clipNr;
      newInstance->setInstanceSettings(instSettings);
    }

    mModelInstData.miAssimpInstances.emplace_back(newInstance);
    mModelInstData.miAssimpInstancesPerModel[model->getModelFileName()].emplace_back(newInstance);
  }
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

  updateTriangleCount();
}

void VkRenderer::updateTriangleCount() {
  mRenderData.rdTriangleCount = 0;
  for (const auto& instance : mModelInstData.miAssimpInstances) {
    mRenderData.rdTriangleCount += instance->getModel()->getTriangleCount();
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
}

void VkRenderer::handleMouseButtonEvents(int button, int action, int mods) {
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  if (button >= 0 && button < ImGuiMouseButton_COUNT) {
    io.AddMouseButtonEvent(button, action == GLFW_PRESS);
  }

  /* hide from application */
  if (io.WantCaptureMouse) {
    return;
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    mMouseLock = !mMouseLock;
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
}

void VkRenderer::handleMousePositionEvents(double xPos, double yPos){
  /* forward to ImGui */
  ImGuiIO& io = ImGui::GetIO();
  io.AddMousePosEvent((float)xPos, (float)yPos);

  /* hide from application */
  if (io.WantCaptureMouse) {
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

  /* save old values*/
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

bool VkRenderer::draw(float deltaTime) {
  mRenderData.rdFrameTime = mFrameTimer.stop();
  mFrameTimer.start();

  handleMovementKeys();

  VkResult result = vkWaitForFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdRenderFence, VK_TRUE, UINT64_MAX);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: waiting for fence failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  result = vkResetFences(mRenderData.rdVkbDevice.device, 1, &mRenderData.rdRenderFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error:  fence reset failed (error: %i)\n", __FUNCTION__, result);
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

  /* here it is safe to delete the Vulkan objects in the pending deletion models */
  for (auto& model : mModelInstData.miPendingDeleteAssimpModels) {
    model->cleanup(mRenderData);
  }
  mModelInstData.miPendingDeleteAssimpModels.clear();

  mMatrixGenerateTimer.start();
  mCamera.updateCamera(mRenderData, deltaTime);

  mMatrices.projectionMatrix = glm::perspective(
    glm::radians(static_cast<float>(mRenderData.rdFieldOfView)),
    static_cast<float>(mRenderData.rdVkbSwapchain.extent.width) / static_cast<float>(mRenderData.rdVkbSwapchain.extent.height),
    0.1f, 500.0f);

  mMatrices.viewMatrix = mCamera.getViewMatrix(mRenderData);
  mRenderData.rdMatrixGenerateTime = mMatrixGenerateTimer.stop();

  mUploadToUBOTimer.start();
  UniformBuffer::uploadData(mRenderData, mPerspectiveViewMatrixUBO, mMatrices);
  mRenderData.rdUploadToUBOTime = mUploadToUBOTimer.stop();

  /* fill the world position matrices */
  mRenderData.rdMatricesSize = 0;

  mWorldPosMatrices.clear();
  mModelBoneMatrices.clear();

  for (const auto& modelType : mModelInstData.miAssimpInstancesPerModel) {
    size_t numberOfInstances = modelType.second.size();
    if (numberOfInstances > 0) {
      std::shared_ptr<AssimpModel> model = modelType.second.at(0)->getModel();

      /* animated models */
      if (model->hasAnimations() &&
        modelType.second.at(0)->getBoneMatrices().size() > 0) {

        mMatrixGenerateTimer.start();
        for (unsigned int i = 0; i < numberOfInstances; ++i) {
          modelType.second.at(i)->updateAnimation(deltaTime);
          std::vector<glm::mat4> instanceBoneMatrices = modelType.second.at(i)->getBoneMatrices();
          mModelBoneMatrices.insert(mModelBoneMatrices.end(),
                                    instanceBoneMatrices.begin(), instanceBoneMatrices.end());
        }

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
      } else {
        /* non-animated models */
        mMatrixGenerateTimer.start();

        for (const auto& instance : modelType.second) {
          mWorldPosMatrices.emplace_back(instance->getWorldTransformMatrix());
        }

        mRenderData.rdMatrixGenerateTime += mMatrixGenerateTimer.stop();
      }
    }
  }

  mRenderData.rdMatricesSize = mModelBoneMatrices.size() * sizeof(glm::mat4) +
    mWorldPosMatrices.size() * sizeof(glm::mat4);

  /* we need to update descriptors after the upload if buffer size changed */
  bool doDescriptorUpdates = false;
  if (mBoneMatrixBuffer.bufferSize < mModelBoneMatrices.size() * sizeof(glm::mat4) ||
      mWorldPosBuffer.bufferSize < mWorldPosMatrices.size() * sizeof(glm::mat4)) {
    doDescriptorUpdates = true;
  }

  mUploadToUBOTimer.start();
  ShaderStorageBuffer::uploadData(mRenderData, mBoneMatrixBuffer, mModelBoneMatrices);
  ShaderStorageBuffer::uploadData(mRenderData, mWorldPosBuffer, mWorldPosMatrices);

  if (doDescriptorUpdates) {
    updateDescriptorSets();
  }
  mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

  /* Vulkan render preparations */
  result = vkResetCommandBuffer(mRenderData.rdCommandBuffer, 0);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to reset command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkCommandBufferBeginInfo cmdBeginInfo{};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  result = vkBeginCommandBuffer(mRenderData.rdCommandBuffer, &cmdBeginInfo);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to begin command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkClearValue colorClearValue;
  colorClearValue.color = { { 0.25f, 0.25f, 0.25f, 1.0f } };

  VkClearValue depthValue;
  depthValue.depthStencil.depth = 1.0f;

  std::vector<VkClearValue> clearValues = { colorClearValue, depthValue };

  VkRenderPassBeginInfo rpInfo{};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpInfo.renderPass = mRenderData.rdRenderpass;

  rpInfo.renderArea.offset.x = 0;
  rpInfo.renderArea.offset.y = 0;
  rpInfo.renderArea.extent = mRenderData.rdVkbSwapchain.extent;
  rpInfo.framebuffer = mRenderData.rdFramebuffers.at(imageIndex);

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

  /* draw the models */
  uint32_t firstAnimatedInstanceToDraw = 0;
  uint32_t firstInstanceToDraw = 0;
  for (const auto& modelType : mModelInstData.miAssimpInstancesPerModel) {
    size_t numberOfInstances = modelType.second.size();
    if (numberOfInstances > 0) {
      std::shared_ptr<AssimpModel> model = modelType.second.at(0)->getModel();

      /* animated models */
      if (model->hasAnimations() &&
        modelType.second.at(0)->getBoneMatrices().size() > 0) {
        size_t numberOfBones = model->getBoneList().size();

        mUploadToUBOTimer.start();
        mModelStride.pkModelStride = numberOfBones;
        vkCmdPushConstants(mRenderData.rdCommandBuffer, mRenderData.rdAssimpSkinningPipelineLayout,
          VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VkPushConstants), &mModelStride);
        mRenderData.rdUploadToUBOTime += mUploadToUBOTimer.stop();

        vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdAssimpSkinningPipeline);

        vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mRenderData.rdAssimpSkinningPipelineLayout, 1, 1, &mRenderData.rdAssimpSkinningDescriptorSet, 0, nullptr);
        model->drawInstanced(mRenderData, numberOfInstances, firstAnimatedInstanceToDraw);
        firstAnimatedInstanceToDraw += numberOfInstances;
      } else {
        /* non-animated models */

        vkCmdBindPipeline(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mRenderData.rdAssimpPipeline);

        vkCmdBindDescriptorSets(mRenderData.rdCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mRenderData.rdAssimpPipelineLayout, 1, 1, &mRenderData.rdAssimpDescriptorSet, 0, nullptr);
        model->drawInstanced(mRenderData, numberOfInstances, firstInstanceToDraw);
        firstInstanceToDraw += numberOfInstances;
      }
    }
  }

  /* imGui overlay */
  mUIGenerateTimer.start();
  mUserInterface.hideMouse(mMouseLock);
  mUserInterface.createFrame(mRenderData, mModelInstData);
  mRenderData.rdUIGenerateTime = mUIGenerateTimer.stop();

  mUIDrawTimer.start();
  mUserInterface.render(mRenderData);
  mRenderData.rdUIDrawTime = mUIDrawTimer.stop();

  vkCmdEndRenderPass(mRenderData.rdCommandBuffer);

  result = vkEndCommandBuffer(mRenderData.rdCommandBuffer);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to end command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  /* submit command buffer */
  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  submitInfo.pWaitDstStageMask = &waitStage;

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &mRenderData.rdPresentSemaphore;

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &mRenderData.rdRenderSemaphore;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &mRenderData.rdCommandBuffer;

  result = vkQueueSubmit(mRenderData.rdGraphicsQueue, 1, &submitInfo, mRenderData.rdRenderFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to submit draw command buffer (%i)\n", __FUNCTION__, result);
    return false;
  }

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

  /* destroy Vulkan objects in models */
  for (const auto& model : mModelInstData.miModelList) {
    model->cleanup(mRenderData);
  }

  mUserInterface.cleanup(mRenderData);

  SyncObjects::cleanup(mRenderData);
  CommandBuffer::cleanup(mRenderData, mRenderData.rdCommandBuffer);
  CommandPool::cleanup(mRenderData);
  Framebuffer::cleanup(mRenderData);

  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpPipeline);
  SkinningPipeline::cleanup(mRenderData, mRenderData.rdAssimpSkinningPipeline);

  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpPipelineLayout);
  PipelineLayout::cleanup(mRenderData, mRenderData.rdAssimpSkinningPipelineLayout);
  Renderpass::cleanup(mRenderData);

  UniformBuffer::cleanup(mRenderData, mPerspectiveViewMatrixUBO);
  ShaderStorageBuffer::cleanup(mRenderData, mBoneMatrixBuffer);
  ShaderStorageBuffer::cleanup(mRenderData, mWorldPosBuffer);

  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpDescriptorSet);
  vkFreeDescriptorSets(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, 1, &mRenderData.rdAssimpSkinningDescriptorSet);

  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpDescriptorLayout, nullptr);
  vkDestroyDescriptorSetLayout(mRenderData.rdVkbDevice.device, mRenderData.rdAssimpTextureDescriptorLayout, nullptr);

  vkDestroyDescriptorPool(mRenderData.rdVkbDevice.device, mRenderData.rdDescriptorPool, nullptr);

  vkDestroyImageView(mRenderData.rdVkbDevice.device, mRenderData.rdDepthImageView, nullptr);

  vmaDestroyImage(mRenderData.rdAllocator, mRenderData.rdDepthImage, mRenderData.rdDepthImageAlloc);
  vmaDestroyAllocator(mRenderData.rdAllocator);

  mRenderData.rdVkbSwapchain.destroy_image_views(mRenderData.rdSwapchainImageViews);
  vkb::destroy_swapchain(mRenderData.rdVkbSwapchain);

  vkb::destroy_device(mRenderData.rdVkbDevice);
  vkb::destroy_surface(mRenderData.rdVkbInstance.instance, mSurface);
  vkb::destroy_instance(mRenderData.rdVkbInstance);

  Logger::log(1, "%s: Vulkan renderer destroyed\n", __FUNCTION__);
}
