/* Vulkan */
#pragma once

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <set>
#include <map>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <assimp/material.h>

#include "Enums.h"
#include "Callbacks.h"
#include "BoundingBox3D.h"

/* morph animations only need those two */
struct VkMorphVertex {
  glm::vec4 position = glm::vec4(0.0f);
  glm::vec4 normal = glm::vec4(0.0f);
};

struct VkMorphMesh {
  std::vector<VkMorphVertex> morphVertices{};
};

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
  /* store optional morph meshes directly in renderer mesh */
  std::vector<VkMorphMesh> morphMeshes{};
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

struct VkSkyboxVertex {
  glm::vec4 position = glm::vec4(0.0f);
};

struct VkSkyboxMesh {
  std::vector<VkSkyboxVertex> vertices{};
};

struct PerInstanceAnimData {
  uint32_t firstAnimClipNum;
  uint32_t secondAnimClipNum;
  uint32_t headLeftRightAnimClipNum;
  uint32_t headUpDownAnimClipNum;
  float firstClipReplayTimestamp;
  float secondClipReplayTimestamp;
  float headLeftRightReplayTimestamp;
  float headUpDownReplayTimestamp;
  float blendFactor;
};

struct MeshTriangle {
  int index;
  std::array<glm::vec3, 3> points{};
  glm::vec3 normal{};
  BoundingBox3D boundingBox{};
  std::array<glm::vec3, 3> edges{};
  std::array<float, 3> edgeLengths{};
};

struct TRSMatrixData{
  glm::vec4 translation{};
  glm::quat rotation{};
  glm::vec4 scale{};
};

struct TimeOfDayLightParameters {
  float timeStamp;
  float lightAngleEW;
  float lightAngleNS;
  float lightIntensity;
  glm::vec3 lightColor{};
};

struct VkUploadMatrices {
  glm::mat4 viewMatrix{};
  glm::mat4 projectionMatrix{};
  glm::mat4 lightAndFogMatrix{};
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
  unsigned int rdLevelTriangleCount = 0;
  unsigned int rdMatricesSize = 0;

  float rdFrameTime = 0.0f;
  float rdMatrixGenerateTime = 0.0f;
  float rdUploadToVBOTime = 0.0f;
  float rdUploadToUBOTime = 0.0f;
  float rdDownloadFromUBOTime = 0.0f;
  float rdUIGenerateTime = 0.0f;
  float rdUIDrawTime = 0.0f;
  float rdCollisionDebugDrawTime = 0.0f;
  float rdCollisionCheckTime = 0.0f;
  float rdBehaviorTime = 0.0f;
  float rdInteractionTime = 0.0f;
  float rdFaceAnimTime = 0.0f;
  float rdLevelCollisionTime = 0.0f;
  float rdIKTime = 0.0f;
  float rdLevelGroundNeighborUpdateTime = 0.0f;
  float rdPathFindingTime = 0.0f;

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
  bool rdShowControlsHelpRequest = false;

  glm::vec3 rdDefaultWorldStartPos = glm::vec3(-160.0f);
  glm::vec3 rdDefaultWorldSize = glm::vec3(320.0f);
  glm::vec3 rdWorldStartPos = rdDefaultWorldStartPos;
  glm::vec3 rdWorldSize = rdDefaultWorldSize;

  collisionChecks rdCheckCollisions = collisionChecks::none;
  size_t rdNumberOfCollisions = 0;

  collisionDebugDraw rdDrawCollisionAABBs = collisionDebugDraw::none;
  collisionDebugDraw rdDrawBoundingSpheres = collisionDebugDraw::none;

  bool rdInteraction = false;
  float rdInteractionMaxRange = 10.0f;
  float rdInteractionMinRange = 1.5f;
  float rdInteractionFOV = 45.0f;
  size_t rdNumberOfInteractionCandidates = 0;
  std::set<int> rdInteractionCandidates{};
  int rdInteractWithInstanceId = 0;

  interactionDebugDraw rdDrawInteractionAABBs = interactionDebugDraw::none;
  bool rdDrawInteractionRange = false;
  bool rdDrawInteractionFOV = false;

  int rdOctreeThreshold = 10;
  int rdOctreeMaxDepth = 5;

  int rdLevelOctreeThreshold = 10;
  int rdLevelOctreeMaxDepth = 5;

  bool rdDrawLevelAABB = false;
  bool rdDrawLevelWireframe = false;
  bool rdDrawLevelOctree = false;
  bool rdDrawLevelCollisionTriangles = false;

  bool rdDrawLevelWireframeMiniMap = false;
  std::shared_ptr<VkLineMesh> rdLevelWireframeMiniMapMesh = nullptr;

  float rdMaxLevelGroundSlopeAngle = 0.0f;
  float rdMaxStairstepHeight = 1.0f;
  glm::vec3 rdLevelCollisionAABBExtension = glm::vec3(0.0f, 1.0f, 0.0f);

  int rdNumberOfCollidingTriangles = 0;
  int rdNumberOfCollidingGroundTriangles = 0;

  bool rdEnableSimpleGravity = false;

  bool rdEnableFeetIK = false;
  int rdNumberOfIkIteratons = 10;
  bool rdDrawIKDebugLines = false;

  bool rdEnableNavigation = false;

  bool rdDrawNeighborTriangles = false;
  bool rdDrawGroundTriangles = false;
  bool rdDrawInstancePaths = false;

  int rdMusicFadeOutSeconds = 0;
  int rdMusicVolume = 0;

  bool rdDrawSkybox = false;

  float rdLightSourceAngleEastWest = 40.0f;
  float rdLightSourceAngleNorthSouth = 40.0f;
  glm::vec3 rdLightSourceColor = glm::vec3(1.0f);
  float rdLightSourceIntensity = 1.0f;

  float rdFogDensity = 0.0f;

  bool rdEnableTimeOfDay = false;

  /* we start at noon */
  float rdTimeOfDay = 720.0f;
  float rdTimeScaleFactor = 10.0f;
  int rdLengthOfDay = 24 * 60;
  timeOfDay rdTimeOfDayPreset = timeOfDay::fullLight;

  std::map<timeOfDay, TimeOfDayLightParameters> rdTimeOfDayLightSettings{};

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
  VkRenderPass rdLevelRenderpass = VK_NULL_HANDLE;

  VkPipelineLayout rdAssimpPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSkinningPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpComputeTransformaPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpComputeMatrixMultPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpComputeBoundingSpheresPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSelectionPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSkinningSelectionPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSkinningMorphPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpSkinningMorphSelectionPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdAssimpLevelPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdLinePipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdSpherePipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdGroundMeshPipelineLayout = VK_NULL_HANDLE;
  VkPipelineLayout rdSkyboxPipelineLayout = VK_NULL_HANDLE;

  VkPipeline rdAssimpPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSkinningPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeTransformPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeHeadMoveTransformPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeMatrixMultPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeBoundingSpheresPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpComputeIKMatrixMultPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSelectionPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSkinningSelectionPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSkinningMorphPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpSkinningMorphSelectionPipeline = VK_NULL_HANDLE;
  VkPipeline rdAssimpLevelPipeline = VK_NULL_HANDLE;
  VkPipeline rdLinePipeline = VK_NULL_HANDLE;
  VkPipeline rdSpherePipeline = VK_NULL_HANDLE;
  VkPipeline rdGroundMeshPipeline = VK_NULL_HANDLE;
  VkPipeline rdSkyboxPipeline = VK_NULL_HANDLE;

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
  VkDescriptorSetLayout rdAssimpSkinningMorphDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpSkinningMorphSelectionDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpSkinningMorphPerModelDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdAssimpLevelDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdLineDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdSphereDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdGroundMeshDescriptorLayout = VK_NULL_HANDLE;
  VkDescriptorSetLayout rdSkyboxDescriptorLayout = VK_NULL_HANDLE;

  VkDescriptorSet rdAssimpDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSkinningDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeTransformDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeMatrixMultDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSelectionDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSkinningSelectionDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSkinningMorphDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpSkinningMorphSelectionDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeSphereTransformDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeSphereMatrixMultDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeBoundingSpheresDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpComputeIKDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdAssimpLevelDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdLineDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdSphereDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdGroundMeshDescriptorSet = VK_NULL_HANDLE;
  VkDescriptorSet rdSkyboxDescriptorSet = VK_NULL_HANDLE;

  VkDescriptorPool rdDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorPool rdImguiDescriptorPool = VK_NULL_HANDLE;
};
