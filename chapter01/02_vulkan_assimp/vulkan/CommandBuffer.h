#pragma once

#include <vulkan/vulkan.h>
#include <VkRenderData.h>

class CommandBuffer {
  public:
    static bool init(VkRenderData renderData, VkCommandBuffer& commandBuffer);
    static void cleanup(VkRenderData renderData, const VkCommandBuffer commandBuffer);
    static VkCommandBuffer createSingleShotBuffer(VkRenderData renderData);
    static bool submitSingleShotBuffer(VkRenderData renderData, const VkCommandBuffer commandBuffer, const VkQueue queue);
};
