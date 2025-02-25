/* Vulkan Pipeline Layout */
#pragma once

#include "VkRenderData.h"

class PipelineLayout {
  public:
    static bool init(VkRenderData& renderData, VkPipelineLayout& pipelineLayout,
      std::vector<VkDescriptorSetLayout> layouts, std::vector<VkPushConstantRange> pushConstants = {});

    static void cleanup(VkRenderData &renderData, VkPipelineLayout &pipelineLayout);
};
