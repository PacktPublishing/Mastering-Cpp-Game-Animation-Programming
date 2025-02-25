#include <vector>

#include <VkBootstrap.h>

#include "ComputePipeline.h"
#include "Logger.h"
#include "Shader.h"

bool ComputePipeline::init(VkRenderData& renderData, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, std::string computeShaderFilename) {
  /* shader */
  VkShaderModule computeModule = Shader::loadShader(renderData.rdVkbDevice.device, computeShaderFilename);

  if (computeModule == VK_NULL_HANDLE) {
      Logger::log(1, "%s error: could not load compute shader\n", __FUNCTION__);
      Shader::cleanup(renderData.rdVkbDevice.device, computeModule);
      return false;
  }
  VkPipelineShaderStageCreateInfo computeStageInfo{};
  computeStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  computeStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  computeStageInfo.module = computeModule;
  computeStageInfo.pName = "main";

  VkComputePipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.stage = computeStageInfo;

  VkResult result = vkCreateComputePipelines(renderData.rdVkbDevice.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create compute pipeline (error: %i)\n", __FUNCTION__, result);
    Shader::cleanup(renderData.rdVkbDevice.device, computeModule);
    return false;
  }

  /* it is save to destroy the shader modules after pipeline has been created */
  Shader::cleanup(renderData.rdVkbDevice.device, computeModule);

  return true;
}


void ComputePipeline::cleanup(VkRenderData& renderData, VkPipeline& pipeline) {
  vkDestroyPipeline(renderData.rdVkbDevice.device, pipeline, nullptr);
}
