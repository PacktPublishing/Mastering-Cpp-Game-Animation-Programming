#include "ShaderStorageBuffer.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool ShaderStorageBuffer::init(VkRenderData& renderData, VkShaderStorageBufferData &SSBOData, size_t bufferSize) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VkResult result = vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &vmaAllocInfo,
    &SSBOData.buffer, &SSBOData.bufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate SSBO via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
  descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
  descriptorAllocateInfo.descriptorSetCount = 1;
  descriptorAllocateInfo.pSetLayouts = &renderData.rdSSBODescriptorLayout;

  result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &descriptorAllocateInfo,
      &SSBOData.descriptorSet);
   if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate SSBO descriptor set (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkDescriptorBufferInfo ssboInfo{};
  ssboInfo.buffer = SSBOData.buffer;
  ssboInfo.offset = 0;
  ssboInfo.range = bufferSize;

  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  writeDescriptorSet.dstSet = SSBOData.descriptorSet;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.pBufferInfo = &ssboInfo;

  vkUpdateDescriptorSets(renderData.rdVkbDevice.device, 1, &writeDescriptorSet, 0, nullptr);

  SSBOData.bufferSize = bufferSize;
  Logger::log(1, "%s: created SSBO of size %i\n", __FUNCTION__, bufferSize);
    return true;
}

void ShaderStorageBuffer::uploadData(VkRenderData &renderData, VkShaderStorageBufferData &SSBOData, std::vector<glm::mat4> bufferData) {
  if (bufferData.size() == 0) {
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
    Logger::log(1, "%s error: coult not map SSBO memory (error: %i)\n", __FUNCTION__, result);
    return;
  }
  std::memcpy(data, bufferData.data(), SSBOData.bufferSize);
  vmaUnmapMemory(renderData.rdAllocator, SSBOData.bufferAlloc);
}

void ShaderStorageBuffer::checkForResize(VkRenderData& renderData, VkShaderStorageBufferData& SSBOData, size_t bufferSize) {
  if (bufferSize > SSBOData.bufferSize) {
    Logger::log(1, "%s: resize SSBO %p from %i to %i bytes\n", __FUNCTION__, SSBOData.buffer, SSBOData.bufferSize, bufferSize);
    cleanup(renderData, SSBOData);
    init(renderData, SSBOData, bufferSize);
  }
}

void ShaderStorageBuffer::cleanup(VkRenderData& renderData, VkShaderStorageBufferData &SSBOData) {
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &SSBOData.descriptorSet);
  vmaDestroyBuffer(renderData.rdAllocator, SSBOData.buffer, SSBOData.bufferAlloc);
}
