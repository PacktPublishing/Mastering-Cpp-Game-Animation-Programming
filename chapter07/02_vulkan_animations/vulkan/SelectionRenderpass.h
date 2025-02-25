/* Renderpass as separate object */
#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class SelectionRenderpass {
  public:
    static bool init(VkRenderData &renderData);
    static void cleanup(VkRenderData &renderData);
};
