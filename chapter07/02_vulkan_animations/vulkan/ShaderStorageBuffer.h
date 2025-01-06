/* Vulkan shader storage buffer object */
#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VkRenderData.h"
#include "Logger.h"

class ShaderStorageBuffer {
  public:
    /* set an arbitraty buffer size as default */
    static bool init(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize = 1024);
    static bool initCoherent(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize = 1024);

    template <typename T>
    static void uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<T> bufferData) {
      if (bufferData.size() == 0) {
        return;
      }

      size_t bufferSize = bufferData.size() * sizeof(T);
      if (bufferSize > SSBOData.bufferSize) {
        Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__, SSBOData.buffer, SSBOData.bufferSize, bufferSize);
        cleanup(renderData, SSBOData);
        init(renderData, SSBOData, bufferSize);
      }

      void* data;
      VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, &data);
      if (result != VK_SUCCESS) {
        Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
        return;
      }
      std::memcpy(data, bufferData.data(), bufferSize);
      vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);
      vmaFlushAllocation(renderData.rdAllocator, SSBOData.bufferAlloc, 0, SSBOData.bufferSize);
    }

    static void checkForResize(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      size_t bufferSize);

    static glm::mat4 getSsboDataMat4(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, size_t offset);

    static void cleanup(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData);
};
