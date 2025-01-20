/* Vulkan compute pipeline with shaders */
#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class ComputePipeline {
  public:
    static bool init(VkRenderData &renderData, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, std::string computeShaderFilename);
    static void cleanup(VkRenderData &renderData, VkPipeline &pipeline);
};
