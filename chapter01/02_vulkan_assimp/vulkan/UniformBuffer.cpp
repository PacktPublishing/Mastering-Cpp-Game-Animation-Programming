#include "UniformBuffer.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool UniformBuffer::init(VkRenderData& renderData, VkUniformBufferData &uboData) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = sizeof(VkUploadMatrices);
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  VmaAllocationCreateInfo vmaAllocInfo{};
  vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VkResult result = vmaCreateBuffer(renderData.rdAllocator, &bufferInfo, &vmaAllocInfo, &uboData.buffer, &uboData.bufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate uniform buffer via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
  descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
  descriptorAllocateInfo.descriptorSetCount = 1;
  descriptorAllocateInfo.pSetLayouts = &renderData.rdUBODescriptorLayout;

  result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &descriptorAllocateInfo, &uboData.descriptorSet);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate UBO descriptor set (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkDescriptorBufferInfo uboInfo{};
  uboInfo.buffer = uboData.buffer;
  uboInfo.offset = 0;
  uboInfo.range = sizeof(VkUploadMatrices);

  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  writeDescriptorSet.dstSet = uboData.descriptorSet;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.pBufferInfo = &uboInfo;

  vkUpdateDescriptorSets(renderData.rdVkbDevice.device, 1, &writeDescriptorSet, 0, nullptr);

  return true;
}

void UniformBuffer::uploadData(VkRenderData &renderData, VkUniformBufferData &uboData, VkUploadMatrices matrices) {
  void* data;
  VkResult result = vmaMapMemory(renderData.rdAllocator, uboData.bufferAlloc, &data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map uniform buffer memory (error: %i)\n", __FUNCTION__, result);
    return;
  }
  std::memcpy(data, &matrices, sizeof(VkUploadMatrices));
  vmaUnmapMemory(renderData.rdAllocator, uboData.bufferAlloc);
}

void UniformBuffer::cleanup(VkRenderData& renderData, VkUniformBufferData &uboData) {
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &uboData.descriptorSet);
  vmaDestroyBuffer(renderData.rdAllocator, uboData.buffer, uboData.bufferAlloc);
}
