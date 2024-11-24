/* Vulkan Pipeline Layout */
#pragma once

#include "VkRenderData.h"

class SkinningPipelineLayout {
  public:
    static bool init(VkRenderData& renderData, VkPipelineLayout& pipelineLayout);
    static void cleanup(VkRenderData &renderData, VkPipelineLayout &pipelineLayout);
};
