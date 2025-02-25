#pragma once

#include <vulkan/vulkan.h>
#include <VkRenderData.h>

class CommandBuffer {
  public:
    static bool init(VkRenderData renderData, VkCommandPool pool, VkCommandBuffer &commandBuffer);

    static bool reset(VkCommandBuffer &commandBuffer, VkCommandBufferResetFlags flags = 0);
    static bool begin(VkCommandBuffer &commandBuffer, VkCommandBufferBeginInfo &beginInfo);
    static bool beginSingleShot(VkCommandBuffer &commandBuffer);
    static bool end(VkCommandBuffer &commandBuffer);

    static VkCommandBuffer createSingleShotBuffer(VkRenderData renderData, VkCommandPool pool);
    static bool submitSingleShotBuffer(VkRenderData renderData, VkCommandPool pool, VkCommandBuffer commandBuffer, VkQueue queue);

    static void cleanup(VkRenderData renderData, VkCommandPool pool, VkCommandBuffer commandBuffer);
};
