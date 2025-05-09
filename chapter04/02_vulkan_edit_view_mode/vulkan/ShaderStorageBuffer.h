/* Vulkan shader storage buffer object */
#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VkRenderData.h"
#include "Logger.h"

class ShaderStorageBuffer {
  public:
    /* set an arbitraty buffer size as default */
    static bool init(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize = 1024);

    template <typename T>
    static bool uploadSsboData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<T> bufferData) {
      if (bufferData.empty()) {
        return false;
      }

      bool bufferResized = false;
      size_t bufferSize = bufferData.size() * sizeof(T);
      if (bufferSize > SSBOData.bufferSize) {
        Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__, SSBOData.buffer, SSBOData.bufferSize, bufferSize);
        cleanup(renderData, SSBOData);
        init(renderData, SSBOData, bufferSize);
        bufferResized = true;
      }

      void* data;
      VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, &data);
      if (result != VK_SUCCESS) {
        Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
        return false;
      }
      std::memcpy(data, bufferData.data(), bufferSize);
      vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);
      vmaFlushAllocation(renderData.rdAllocator, SSBOData.bufferAlloc, 0, SSBOData.bufferSize);

      return bufferResized;
    }

    static bool checkForResize(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      size_t bufferSize);

    static void cleanup(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData);
};
