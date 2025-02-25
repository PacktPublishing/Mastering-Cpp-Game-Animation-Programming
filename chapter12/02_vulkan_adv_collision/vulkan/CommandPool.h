/* Vulkan command pool */
#pragma once

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>

#include "VkRenderData.h"

class CommandPool {
  public:
    static bool init(VkRenderData &renderData, vkb::QueueType queueType, VkCommandPool &pool);

    static void cleanup(VkRenderData &renderData, VkCommandPool pool);
};
