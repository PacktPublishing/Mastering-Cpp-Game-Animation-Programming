/* Vulkan shader storage buffer object */
#pragma once

#include <vector>
#include <cstdint>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VkRenderData.h"

class ShaderStorageBuffer {
  public:
    /* set an arbitraty buffer size as default */
    static bool init(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize = 1024);

    static void uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      std::vector<glm::mat4> bufferData);
    static void uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      std::vector<int32_t> bufferData);
    static void uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      std::vector<NodeTransformData> bufferData);

    static void checkForResize(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      size_t bufferSize);

    static void cleanup(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData);
};
