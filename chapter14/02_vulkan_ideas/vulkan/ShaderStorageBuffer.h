/* Vulkan shader storage buffer object */
#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "VkRenderData.h"
#include "AABB.h"
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

    template <typename T>
    static void uploadSsboData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<T> bufferData,
        int offset) {
      if (bufferData.empty()) {
        return;
      }

      size_t bufferSize = bufferData.size() * sizeof(T);
      if (bufferSize > SSBOData.bufferSize) {
        Logger::log(1, "%s error: resize SSBO %p would re-init buffer\n", __FUNCTION__, SSBOData.buffer);
        return;
      }

      T* data;
      VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, (void**)&data);
      if (result != VK_SUCCESS) {
        Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
        return;
      }
      std::memcpy(data + offset, bufferData.data() + offset, bufferSize - offset * sizeof(T));
      vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);
      vmaFlushAllocation(renderData.rdAllocator, SSBOData.bufferAlloc, 0, SSBOData.bufferSize);
    }

    static bool checkForResize(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      size_t bufferSize);

    size_t getBufferSize(VkShaderStorageBufferData &SSBOData);

    static glm::mat4 getSsboDataMat4(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, size_t offset);
    static std::vector<glm::mat4> getSsboDataMat4(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData);
    static std::vector<glm::mat4> getSsboDataMat4(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      size_t offset, int numberOfElements);
    static std::vector<glm::vec4> getSsboDataVec4(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, int numberOfElements);
    static std::vector<TRSMatrixData> getSsboDataTRSMatrixData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData);
    static std::vector<TRSMatrixData> getSsboDataTRSMatrixData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData,
      size_t offset, int numberOfElements);

    static void cleanup(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData);
};
