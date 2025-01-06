/* Renderpass as separate object */
#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class Renderpass {
  public:
    static bool init(VkRenderData &renderData, VkRenderPass &renderPass);
    static void cleanup(VkRenderData &renderData, VkRenderPass renderPass);
};
