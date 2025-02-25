#include "PipelineLayout.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool PipelineLayout::init(VkRenderData& renderData, VkPipelineLayout& pipelineLayout,
    std::vector<VkDescriptorSetLayout> layouts, std::vector<VkPushConstantRange> pushConstants) {

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
  pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();

  VkResult result = vkCreatePipelineLayout(renderData.rdVkbDevice.device, &pipelineLayoutInfo, nullptr, &pipelineLayout);
  if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: could not create pipeline layout (error: %i)\n", __FUNCTION__, result);
      return false;
  }
  return true;
}

void PipelineLayout::cleanup(VkRenderData &renderData, VkPipelineLayout &pipelineLayout) {
  vkDestroyPipelineLayout(renderData.rdVkbDevice.device, pipelineLayout, nullptr);
}
