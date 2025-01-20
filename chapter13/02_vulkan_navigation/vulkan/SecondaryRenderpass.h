/* Secondary renderpass, draws on top of another renderpass */
#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class SecondaryRenderpass {
  public:
    static bool init(VkRenderData &renderData, VkRenderPass &renderPass);
    static void cleanup(VkRenderData &renderData, VkRenderPass renderPass);
};
