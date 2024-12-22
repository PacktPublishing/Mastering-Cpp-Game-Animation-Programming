/* Vulkan uniform buffer object */
#pragma once

#include <vulkan/vulkan.h>

#include "VkRenderData.h"

class UniformBuffer {
  public:
    static bool init(VkRenderData &renderData, VkUniformBufferData &uboData);
    static void uploadData(VkRenderData &renderData, VkUniformBufferData &uboData, VkUploadMatrices matrices);
    static void cleanup(VkRenderData &renderData, VkUniformBufferData &uboData);
};
