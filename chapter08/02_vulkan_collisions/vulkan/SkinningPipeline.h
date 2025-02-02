/* Vulkan graphics pipeline with shaders */
#pragma once

#include <string>
#include <cstdint>
#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class SkinningPipeline {
  public:
    static bool init(VkRenderData &renderData, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline,
      VkRenderPass renderpass, uint32_t numColorAttachements,
      std::string vertexShaderFilename, std::string fragmentShaderFilename);
    static void cleanup(VkRenderData &renderData, VkPipeline &pipeline);
};
