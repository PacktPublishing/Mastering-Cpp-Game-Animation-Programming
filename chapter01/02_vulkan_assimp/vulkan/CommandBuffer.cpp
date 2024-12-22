#include <CommandBuffer.h>
#include <Logger.h>
#include <VkBootstrap.h>

bool CommandBuffer::init(VkRenderData renderData, VkCommandBuffer& commandBuffer) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = renderData.rdCommandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;

  VkResult result = vkAllocateCommandBuffers(renderData.rdVkbDevice.device, &allocInfo, &commandBuffer);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  return true;
}

bool CommandBuffer::reset(VkCommandBuffer &commandBuffer, VkCommandBufferResetFlags flags) {
  VkResult result = vkResetCommandBuffer(commandBuffer, flags);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not reset command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  return true;
}

bool CommandBuffer::begin(VkCommandBuffer &commandBuffer, VkCommandBufferBeginInfo &beginInfo) {
  VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not begin new command buffer\n", __FUNCTION__);
    return false;
  }
  return true;
}

bool CommandBuffer::beginSingleShot(VkCommandBuffer& commandBuffer) {
  VkCommandBufferBeginInfo cmdBeginInfo{};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  VkResult result = vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not begin new command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  return true;
}

bool CommandBuffer::end(VkCommandBuffer& commandBuffer) {
  VkResult result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not end render pass (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  return true;
}

VkCommandBuffer CommandBuffer::createSingleShotBuffer(VkRenderData renderData) {
  Logger::log(2, "%s: creating a single shot command buffer\n", __FUNCTION__);
  VkCommandBuffer commandBuffer;

  if (!init(renderData, commandBuffer)) {
    Logger::log(1, "%s error: could not create command buffer\n", __FUNCTION__);
    return VK_NULL_HANDLE;
  }

  VkResult result = vkResetCommandBuffer(commandBuffer, 0);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to reset command buffer (error: %i)\n", __FUNCTION__, result);
    return VK_NULL_HANDLE;
  }

  VkCommandBufferBeginInfo cmdBeginInfo{};
  cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  result = vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to begin command buffer (error: %i)\n", __FUNCTION__, result);
    return VK_NULL_HANDLE;
  }

  Logger::log(2, "%s: single shot command buffer successfully created\n", __FUNCTION__);
  return commandBuffer;
}

bool CommandBuffer::submitSingleShotBuffer(VkRenderData renderData, VkCommandBuffer commandBuffer, VkQueue queue) {
  Logger::log(2, "%s: submitting single shot command buffer\n", __FUNCTION__);

  VkResult result = vkEndCommandBuffer(commandBuffer);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to end command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  VkFence bufferFence;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  result = vkCreateFence(renderData.rdVkbDevice.device, &fenceInfo, nullptr, &bufferFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to create buffer fence (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  result = vkResetFences(renderData.rdVkbDevice.device, 1, &bufferFence);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: buffer fence reset failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

   result = vkQueueSubmit(queue, 1, &submitInfo, bufferFence);
   if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: failed to submit buffer copy command buffer (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  result = vkWaitForFences(renderData.rdVkbDevice.device, 1, &bufferFence, VK_TRUE, UINT64_MAX);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: waiting for buffer fence failed (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  vkDestroyFence(renderData.rdVkbDevice.device, bufferFence, nullptr);
  cleanup(renderData, commandBuffer);

  Logger::log(2, "%s: single shot command buffer successfully submitted\n", __FUNCTION__);
  return true;
}

void CommandBuffer::cleanup(VkRenderData renderData, VkCommandBuffer commandBuffer) {
  vkFreeCommandBuffers(renderData.rdVkbDevice.device, renderData.rdCommandPool, 1, &commandBuffer);
}
