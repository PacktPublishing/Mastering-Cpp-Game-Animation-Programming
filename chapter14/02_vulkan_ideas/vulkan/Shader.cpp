#include "Shader.h"
#include "Tools.h"
#include "Logger.h"

VkShaderModule Shader::loadShader(VkDevice device, std::string shaderFileName) {
  std::string shaderAsText;
  shaderAsText = Tools::loadFileToString(shaderFileName);

  VkShaderModuleCreateInfo shaderCreateInfo{};
  shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderCreateInfo.codeSize = shaderAsText.size();
  /* casting needed to align the code to 32 bit */
  shaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderAsText.c_str());

  VkShaderModule shaderModule;
  VkResult result = vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s: could not load shader '%s' (error: %i)\n", __FUNCTION__, shaderFileName.c_str(), result);
    return VK_NULL_HANDLE;
  }

  return shaderModule;
}

void Shader::cleanup(VkDevice device, VkShaderModule module) {
  vkDestroyShaderModule(device, module, nullptr);
}
