#include "ShaderStorageBuffer.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool ShaderStorageBuffer::init(VkRenderData& renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize) {
  /* avoid errors with zero sized buffers */
  if (bufferSize == 0) {
    bufferSize = 1024;
  }

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  VkResult result = vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &vmaAllocInfo,
    &SSBOData.buffer, &SSBOData.bufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate SSBO via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  SSBOData.bufferSize = bufferSize;
  Logger::log(1, "%s: created SSBO of size %i\n", __FUNCTION__, bufferSize);
    return true;
}

bool ShaderStorageBuffer::initCoherent(VkRenderData& renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize) {
  /* avoid errors with zero sized buffers */
  if (bufferSize == 0) {
    bufferSize = 1024;
  }

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  vmaAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VkResult result = vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &vmaAllocInfo,
    &SSBOData.buffer, &SSBOData.bufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate SSBO via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  SSBOData.bufferSize = bufferSize;
  Logger::log(1, "%s: created SSBO of size %i\n", __FUNCTION__, bufferSize);
    return true;
}

void ShaderStorageBuffer::uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<glm::mat4> bufferData) {
  if (bufferData.empty()) {
    return;
  }

  size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
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

void ShaderStorageBuffer::uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<int32_t> bufferData) {
  if (bufferData.empty()) {
    return;
  }

  size_t bufferSize = bufferData.size() * sizeof(int32_t);
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

void ShaderStorageBuffer::uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<NodeTransformData> bufferData) {
  if (bufferData.empty()) {
    return;
  }

  size_t bufferSize = bufferData.size() * sizeof(NodeTransformData);
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

void ShaderStorageBuffer::uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<glm::vec2> bufferData) {
  if (bufferData.empty()) {
    return;
  }

  size_t bufferSize = bufferData.size() * sizeof(glm::vec2);
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

void ShaderStorageBuffer::checkForResize(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData, size_t bufferSize) {
  if (bufferSize > SSBOData.bufferSize) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__, SSBOData.buffer, SSBOData.bufferSize, bufferSize);
    cleanup(renderData, SSBOData);
    init(renderData, SSBOData, bufferSize);
  }
}

glm::mat4 ShaderStorageBuffer::getSsboDataMat4(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData, size_t offset) {
  glm::mat4 resultMatrix = glm::mat4(1.0f);

  glm::mat4* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, (void**)&data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
    return resultMatrix;
  }
  std::memcpy(&resultMatrix, data + offset, sizeof(glm::mat4));
  vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);

  return resultMatrix;
}

void ShaderStorageBuffer::cleanup(VkRenderData& renderData, VkShaderStorageBufferData &SSBOData) {
  VkResult result = vkQueueWaitIdle(renderData.rdGraphicsQueue);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s fatal error: could not wait for device idle (error: %i)\n", __FUNCTION__, result);
  }
  vmaDestroyBuffer(renderData.rdAllocator, SSBOData.buffer, SSBOData.bufferAlloc);
}
