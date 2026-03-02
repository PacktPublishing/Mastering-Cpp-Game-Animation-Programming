/* Vulkan framebuffer class */
#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class SelectionFramebuffer {
  public:
    static bool init(VkRenderData &renderData);
    static float getPixelValueFromPos(VkRenderData &renderData, int xPos, int yPos);
    static void cleanup(VkRenderData &renderData);
};
