#include "SkinningPipelineLayout.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool SkinningPipelineLayout::init(VkRenderData& renderData, VkPipelineLayout& pipelineLayout) {
  VkPushConstantRange pushConstants{};
  pushConstants.offset = 0;
  pushConstants.size = sizeof(VkPushConstants);
  pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  std::vector<VkDescriptorSetLayout> layouts = { renderData.rdTextureDescriptorLayout,
    renderData.rdUBODescriptorLayout,
    renderData.rdSSBODescriptorLayout,
    renderData.rdDynamicSSBODescriptorLayout };

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstants;

  VkResult result = vkCreatePipelineLayout(renderData.rdVkbDevice.device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
  if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create pipeline layout (error: %i)\n", __FUNCTION__, result);
      return false;
  }
  return true;
}

void SkinningPipelineLayout::cleanup(VkRenderData &renderData, VkPipelineLayout &pipelineLayout) {
  vkDestroyPipelineLayout(renderData.rdVkbDevice.device, pipelineLayout, nullptr);
}
