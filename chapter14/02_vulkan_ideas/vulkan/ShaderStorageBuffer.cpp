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

bool ShaderStorageBuffer::checkForResize(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData, size_t bufferSize) {
  if (bufferSize > SSBOData.bufferSize) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__, SSBOData.buffer, SSBOData.bufferSize, bufferSize);
    cleanup(renderData, SSBOData);
    init(renderData, SSBOData, bufferSize);
    return true;
  }
  return false;
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

std::vector<glm::mat4> ShaderStorageBuffer::getSsboDataMat4(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData) {
  std::vector<glm::mat4> resultMatrices{};

  glm::mat4* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, (void**)&data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
    return resultMatrices;
  }
  resultMatrices.resize(SSBOData.bufferSize / sizeof(glm::mat4));
  std::memcpy(resultMatrices.data(), data, SSBOData.bufferSize);
  vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);

  return resultMatrices;
}

std::vector<glm::mat4> ShaderStorageBuffer::getSsboDataMat4(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData,
    size_t offset, int numberOfElements) {
  std::vector<glm::mat4> resultMatrices{};

  glm::mat4* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, (void**)&data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
    return resultMatrices;
  }

  resultMatrices.resize(numberOfElements);
  std::memcpy(resultMatrices.data(), data + offset, numberOfElements * sizeof(glm::mat4));
  vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);

  return resultMatrices;
}

std::vector<glm::vec4> ShaderStorageBuffer::getSsboDataVec4(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData, int numberOfElements) {
  std::vector<glm::vec4> resultVec{};

  glm::vec4* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, (void**)&data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
    return resultVec;
  }
  resultVec.resize(numberOfElements);
  std::memcpy(resultVec.data(), data, numberOfElements * sizeof(glm::vec4));
  vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);

  return resultVec;
}

std::vector<TRSMatrixData> ShaderStorageBuffer::getSsboDataTRSMatrixData(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData) {
  std::vector<TRSMatrixData> resultVec{};

  TRSMatrixData* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, (void**)&data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
    return resultVec;
  }
  resultVec.resize(SSBOData.bufferSize / sizeof(TRSMatrixData));
  std::memcpy(resultVec.data(), data, SSBOData.bufferSize);
  vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);

  return resultVec;
}

std::vector<TRSMatrixData> ShaderStorageBuffer::getSsboDataTRSMatrixData(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData,
    size_t offset, int numberOfElements) {
  std::vector<TRSMatrixData> resultVec{};

  TRSMatrixData* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, SSBOData.bufferAlloc, (void**)&data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map SSBO memory (error: %i)\n", __FUNCTION__, result);
    return resultVec;
  }

  resultVec.resize(numberOfElements);
  std::memcpy(resultVec.data(), data + offset, numberOfElements * sizeof(TRSMatrixData));
  vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);

  return resultVec;
}

void ShaderStorageBuffer::cleanup(VkRenderData& renderData, VkShaderStorageBufferData &SSBOData) {
  VkResult result = vkQueueWaitIdle(renderData.rdGraphicsQueue);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s fatal error: could not wait for device idle (error: %i)\n", __FUNCTION__, result);
  }
  vmaDestroyBuffer(renderData.rdAllocator, SSBOData.buffer, SSBOData.bufferAlloc);
}

size_t ShaderStorageBuffer::getBufferSize(VkShaderStorageBufferData& SSBOData) {
  return SSBOData.bufferSize;
}

