#include <cstring>

#include "IndexBuffer.h"
#include "CommandBuffer.h"
#include "Logger.h"

bool IndexBuffer::init(VkRenderData &renderData, VkIndexBufferData &bufferData,
  size_t bufferSize) {
  /* index buffer */
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo bufferAllocInfo{};
  bufferAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VkResult result =vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &bufferAllocInfo,
      &bufferData.buffer, &bufferData.bufferAlloc, nullptr);
  if ( result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate index buffer via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  /* staging buffer for copy */
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size = bufferSize;;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  result = vmaCreateBuffer(renderData.rdAllocator, &stagingBufferInfo, &stagingAllocInfo,
      &bufferData.stagingBuffer, &bufferData.stagingBufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate index staging buffer via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  bufferData.bufferSize = bufferSize;
  return true;
}

bool IndexBuffer::uploadData(VkRenderData& renderData, VkIndexBufferData& bufferData, VkMesh vertexData) {
  /* buffer too small, resize */
  unsigned int indexDataSize = vertexData.indices.size() * sizeof(uint32_t);
  if (bufferData.bufferSize < indexDataSize) {
    cleanup(renderData, bufferData);

    if (!init(renderData, bufferData, indexDataSize)) {
      Logger::log(1, "%s error: could not create index buffer of size %i bytes\n", __FUNCTION__, indexDataSize);
      return false;
    }
    Logger::log(1, "%s: index buffer resize to %i bytes\n", __FUNCTION__, indexDataSize);
    bufferData.bufferSize = indexDataSize;
  }

  /* copy data to staging buffer */
  void* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, bufferData.stagingBufferAlloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map index buffer memory (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  std::memcpy(data, vertexData.indices.data(), indexDataSize);
  vmaUnmapMemory(renderData.rdAllocator, bufferData.stagingBufferAlloc);
  vmaFlushAllocation(renderData.rdAllocator, bufferData.stagingBufferAlloc, 0, indexDataSize);

  VkBufferMemoryBarrier indexBufferBarrier{};
  indexBufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
  indexBufferBarrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
  indexBufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  indexBufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  indexBufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  indexBufferBarrier.buffer = bufferData.stagingBuffer;
  indexBufferBarrier.offset = 0;
  indexBufferBarrier.size = bufferData.bufferSize;

  VkBufferCopy stagingBufferCopy{};
  stagingBufferCopy.srcOffset = 0;
  stagingBufferCopy.dstOffset = 0;
  stagingBufferCopy.size = bufferData.bufferSize;

  /* trigger data transfer via command buffer */
  VkCommandBuffer commandBuffer = CommandBuffer::createSingleShotBuffer(renderData, renderData.rdCommandPool);

  vkCmdCopyBuffer(commandBuffer, bufferData.stagingBuffer,
                  bufferData.buffer, 1, &stagingBufferCopy);
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &indexBufferBarrier, 0, nullptr);

  if (!CommandBuffer::submitSingleShotBuffer(renderData, renderData.rdCommandPool, commandBuffer, renderData.rdGraphicsQueue)) {
    return false;
  }

  return true;
}

void IndexBuffer::cleanup(VkRenderData &renderData, VkIndexBufferData &bufferData) {
  vmaDestroyBuffer(renderData.rdAllocator, bufferData.stagingBuffer,
    bufferData.stagingBufferAlloc);
  vmaDestroyBuffer(renderData.rdAllocator, bufferData.buffer,
    bufferData.bufferAlloc);
}
