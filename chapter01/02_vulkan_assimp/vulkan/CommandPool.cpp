#include "CommandPool.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool CommandPool::init(VkRenderData &renderData) {
  VkCommandPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolCreateInfo.queueFamilyIndex = renderData.rdVkbDevice.get_queue_index(vkb::QueueType::graphics).value();
  poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkResult result = vkCreateCommandPool(renderData.rdVkbDevice.device, &poolCreateInfo, nullptr, &renderData.rdCommandPool);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create command pool (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  return true;
}

void CommandPool::cleanup(VkRenderData &renderData) {
  vkDestroyCommandPool(renderData.rdVkbDevice.device, renderData.rdCommandPool, nullptr);
}
