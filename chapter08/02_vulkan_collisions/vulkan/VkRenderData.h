/* Vulkan */
#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include <glm/glm.hpp>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <assimp/material.h>

#include "Enums.h"
#include "Callbacks.h"

struct VkVertex {
  glm::vec4 position = glm::vec4(0.0f); // last float is uv.x
  glm::vec4 color = glm::vec4(1.0f);
  glm::vec4 normal = glm::vec4(0.0f); // last float is uv.y
  glm::uvec4 boneNumber = glm::uvec4(0);
  glm::vec4 boneWeight = glm::vec4(0.0f);
};

struct VkMesh {
  std::vector<VkVertex> vertices{};
  std::vector<uint32_t> indices{};
  std::unordered_map<aiTextureType, std::string> textures{};
  bool usesPBRColors = false;
};

struct VkLineVertex {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 color = glm::vec3(0.0f);

  VkLineVertex() {};
  VkLineVertex(glm::vec3 pos, glm::vec3 col) : position(pos), color(col) {};
};

struct VkLineMesh {
  std::vector<VkLineVertex> vertices{};
};

struct PerInstanceAnimData {
  uint32_t firstAnimClipNum;
  uint32_t secondAnimClipNum;
  float firstClipReplayTimestamp;
  float secondClipReplayTimestamp;
  float blendFactor;
};

struct VkUploadMatrices {
  glm::mat4 viewMatrix{};
  glm::mat4 projectionMatrix{};
};

struct VkTextureData {
  VkImage image = VK_NULL_HANDLE;
  VkImageView imageView = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VmaAllocation imageAlloc = nullptr;

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct VkVertexBufferData {
  unsigned int bufferSize = 0;
  void* data = nullptr;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation bufferAlloc = VK_NULL_HANDLE;
  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VmaAllocation stagingBufferAlloc = VK_NULL_HANDLE;
};

struct VkIndexBufferData {
  size_t bufferSize = 0;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation bufferAlloc = nullptr;
  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VmaAllocation stagingBufferAlloc = nullptr;
};

struct VkUniformBufferData {
  size_t bufferSize = 0;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation bufferAlloc = nullptr;

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct VkShaderStorageBufferData {
  size_t bufferSize = 0;
  VkBuffer buffer = VK_NULL_HANDLE;
  VmaAllocation bufferAlloc = nullptr;

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};

struct VkPushConstants {
  uint32_t pkModelStride;
  uint32_t pkWorldPosOffset;
  uint32_t pkSkinMatOffset;
};

struct VkComputePushConstants {
  uint32_t pkModelOffset;
  uint32_t pkInstanceOffset;
};
struct VkRenderData {
  GLFWwindow *rdWindow = nullptr;

  int rdWidth = 0;
  int rdHeight = 0;
  bool rdFullscreen = false;

  unsigned int rdTriangleCount = 0;
  unsigned int rdMatricesSize = 0;

  float rdFrameTime = 0.0f;
  float rdMatrixGenerateTime = 0.0f;
  float rdUploadToVBOTime = 0.0f;
  float rdUploadToUBOTime = 0.0f;
  float rdUIGenerateTime = 0.0f;
  float rdUIDrawTime = 0.0f;
  float rdCollisionDebugDrawTime = 0.0f;
  float rdCollisionCheckTime = 0.0f;

  int rdMoveForward = 0;
  int rdMoveRight = 0;
  int rdMoveUp = 0;

  bool rdHighlightSelectedInstance = false;
  float rdSelectedInstanceHighlightValue = 1.0f;

  appMode rdApplicationMode = appMode::edit;
  std::unordered_map<appMode, std::string> mAppModeMap{};

  instanceEditMode rdInstanceEditMode = instanceEditMode::move;

  appExitCallback rdAppExitCallbackFunction;
  bool rdRequestApplicationExit = false;
  bool rdNewConfigRequest = false;
  bool rdLoadConfigRequest = false;
  bool rdSaveConfigRequest = false;

  glm::vec2 rdWorldStartPos = glm::vec2(-128.0f);
  glm::vec2 rdWorldSize = glm::vec2(256.0f);

  collisionChecks rdCheckCollisions = collisionChecks::none;
  size_t rdNumberOfCollisions = 0;

  collisionDebugDraw rdDrawCollisionAABBs = collisionDebugDraw::none;
  collisionDebugDraw rdDrawBoundingSpheres = collisionDebugDraw::none;

  /* Vulkan specific stuff */
  VmaAllocator rdAllocator = nullptr;

  vkb::Instance rdVkbInstance{};
  vkb::PhysicalDevice rdVkbPhysicalDevice{};
  vkb::Device rdVkbDevice{};
  vkb::Swapchain rdVkbSwapchain{};

  std::vector<VkImage> rdSwapchainImages{};
  std::vector<VkImageView> rdSwapchainImageViews{};
  std::vector<VkFramebuffer> rdFramebuffers{};
  std::vector<VkFramebuffer> rdSelectionFramebuffers{};

  VkQueue rdGraphicsQueue = VK_NULL_HANDLE;
  VkQueue rdPresentQueue = VK_NULL_HANDLE;
  VkQueue rdComputeQueue = VK_NULL_HANDLE;

  VkImage rdDepthImage = VK_NULL_HANDLE;
  VkImageView rdDepthImageView = VK_NULL_HANDLE;
  VkFormat rdDepthFormat = VK_FORMAT_UNDEFINED;
  VmaAllocation rdDepthImageAlloc = VK_NULL_HANDLE;

  VkImage rdSelectionImage = VK_NULL_HANDLE;
  VkImageView rdSelectionImageView = VK_NULL_HANDLE;
  VkFormat rdSelectionFormat = VK_FORMAT_UNDEFINED;
  VmaAllocation rdSelectionImageAlloc = VK_NULL_HANDLE;

  VkRenderPass rdRenderpass = VK_NULL_HANDLE;
  VkRenderPass rdImGuiRenderpass = VK_NULL_HANDLE;
  VkRenderPass rdSelectionRenderpass = VK_NULL_HANDLE;
  VkRenderPass rdLineRenderpass = VK_NULL_HANDLE;

  VkPipelineLayout rdAssimpPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSkinningPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpComputeTransformaPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpComputeMatrixMultPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpComputeBoundingSpheresPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSelectionPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSkinningSelectionPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdLinePipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdSpherePipelineLayout = VK_NULL_HANDLE;

  VkPipeline rdAssimpPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSkinningPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeTransformPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeMatrixMultPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeBoundingSpheresPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSelectionPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSkinningSelectionPipeline = VK_NULL_HANDLE;
  VkPipeline rdLinePipeline = VK_NULL_HANDLE;
  VkPipeline rdSpherePipeline = VK_NULL_HANDLE;

  VkCommandPool rdCommandPool = VK_NULL_HANDLE;
  VkCommandPool rdComputeCommandPool = VK_NULL_HANDLE;
  VkCommandBuffer rdCommandBuffer = VK_NULL_HANDLE;
  VkCommandBuffer rdImGuiCommandBuffer = VK_NULL_HANDLE;
  VkCommandBuffer rdLineCommandBuffer = VK_NULL_HANDLE;
  VkCommandBuffer rdComputeCommandBuffer = VK_NULL_HANDLE;

  VkSemaphore rdPresentSemaphore = VK_NULL_HANDLE;
  VkSemaphore rdRenderSemaphore = VK_NULL_HANDLE;
  VkSemaphore rdGraphicSemaphore = VK_NULL_HANDLE;
  VkSemaphore rdComputeSemaphore = VK_NULL_HANDLE;
  VkSemaphore rdCollisionSemaphore = VK_NULL_HANDLE;
  VkFence rdRenderFence = VK_NULL_HANDLE;
  VkFence rdComputeFence = VK_NULL_HANDLE;
  VkFence rdCollisionFence = VK_NULL_HANDLE;

  VkDescriptorSetLayout rdAssimpDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpSkinningDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpTextureDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpComputeTransformDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpComputeTransformPerModelDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpComputeMatrixMultDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpComputeMatrixMultPerModelDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpComputeBoundingSpheresDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpComputeBoundingSpheresPerModelDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpSelectionDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpSkinningSelectionDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdLineDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdSphereDescriptorLayout = VK_NULL_HANDLE;

  VkDescriptorSet rdAssimpDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSkinningDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeTransformDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeMatrixMultDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSelectionDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSkinningSelectionDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeSphereTransformDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeSphereMatrixMultDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeBoundingSpheresDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdLineDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdSphereDescriptorSet = VK_NULL_HANDLE;

  VkDescriptorPool rdDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorPool rdImguiDescriptorPool = VK_NULL_HANDLE;
};
