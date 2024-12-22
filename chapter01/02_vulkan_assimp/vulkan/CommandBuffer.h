#pragma once

#include <vulkan/vulkan.h>
#include <VkRenderData.h>

class CommandBuffer {
  public:
    static bool init(VkRenderData renderData, VkCommandBuffer& commandBuffer);

    static bool reset(VkCommandBuffer &commandBuffer, VkCommandBufferResetFlags flags = 0);
    static bool begin(VkCommandBuffer &commandBuffer, VkCommandBufferBeginInfo &beginInfo);
    static bool beginSingleShot(VkCommandBuffer &commandBuffer);
    static bool end(VkCommandBuffer &commandBuffer);

    static VkCommandBuffer createSingleShotBuffer(VkRenderData renderData);
    static bool submitSingleShotBuffer(VkRenderData renderData, VkCommandBuffer commandBuffer, VkQueue queue);

    static void cleanup(VkRenderData renderData, VkCommandBuffer commandBuffer);
};
