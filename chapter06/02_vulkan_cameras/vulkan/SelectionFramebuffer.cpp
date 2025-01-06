#include "SelectionFramebuffer.h"
#include "CommandBuffer.h"
#include "Logger.h"

bool SelectionFramebuffer::init(VkRenderData &renderData) {
  renderData.rdSelectionFramebuffers.resize(renderData.rdSwapchainImageViews.size());

  for (unsigned int i = 0; i < renderData.rdSwapchainImageViews.size(); ++i) {
    std::vector<VkImageView> attachments = { renderData.rdSwapchainImageViews.at(i),
      renderData.rdSelectionImageView, renderData.rdDepthImageView };

    VkFramebufferCreateInfo FboInfo{};
    FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FboInfo.renderPass = renderData.rdSelectionRenderpass;
    FboInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    FboInfo.pAttachments = attachments.data();
    FboInfo.width = renderData.rdVkbSwapchain.extent.width;
    FboInfo.height = renderData.rdVkbSwapchain.extent.height;
    FboInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(renderData.rdVkbDevice.device, &FboInfo, nullptr, &renderData.rdSelectionFramebuffers.at(i));
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to create selection framebuffer %i (error: %i)\n", __FUNCTION__, i, result);
      return false;
    }
  }
  return true;
}

float SelectionFramebuffer::getPixelValueFromPos(VkRenderData& renderData, unsigned int xPos, unsigned int yPos) {
  /* random default value to detect errors */
  float pixelColor = -444.0f;

  VkImage readbackImage;
  VmaAllocation readbackImageAlloc;

  /* create local image */
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = static_cast<uint32_t>(renderData.rdVkbSwapchain.extent.width);
  imageInfo.extent.height = static_cast<uint32_t>(renderData.rdVkbSwapchain.extent.height);
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R32_SFLOAT;
  imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  VmaAllocationCreateInfo imageAllocInfo{};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
  imageAllocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  VkResult result = vmaCreateImage(renderData.rdAllocator, &imageInfo, &imageAllocInfo, &readbackImage,  &readbackImageAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate read back image image via VMA (error: %i)\n", __FUNCTION__, result);
    return pixelColor;
  }

  VkCommandBuffer readbackCommandBuffer = CommandBuffer::createSingleShotBuffer(renderData, renderData.rdCommandPool);

  VkImageSubresourceRange layoutTransferRange{};
  layoutTransferRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  layoutTransferRange.baseMipLevel = 0;
  layoutTransferRange.levelCount = 1;
  layoutTransferRange.baseArrayLayer = 0;
  layoutTransferRange.layerCount = 1;

  /* transition destination (local) image to transfer destination layout */
  VkImageMemoryBarrier layoutTransferBarrier{};
  layoutTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  layoutTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  layoutTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  layoutTransferBarrier.image = readbackImage;
  layoutTransferBarrier.subresourceRange = layoutTransferRange;
  layoutTransferBarrier.srcAccessMask = 0;
  layoutTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  /* transition source (selection) image to transfer source optimal layout */
  VkImageMemoryBarrier srcLayoutTransferBarrier{};
  srcLayoutTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  srcLayoutTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  srcLayoutTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  srcLayoutTransferBarrier.image = renderData.rdSelectionImage;
  srcLayoutTransferBarrier.subresourceRange = layoutTransferRange;
  srcLayoutTransferBarrier.srcAccessMask = 0;
  srcLayoutTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  /* copy selection image to local image */
  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.extent.width = static_cast<uint32_t>(renderData.rdVkbSwapchain.extent.width);
  imageCopyRegion.extent.height = static_cast<uint32_t>(renderData.rdVkbSwapchain.extent.height);
  imageCopyRegion.extent.depth = 1;

  /* transition destination (local) image to general layout to allow mapping */
  VkImageMemoryBarrier destLayoutTransferBarrier{};
  destLayoutTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  destLayoutTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  destLayoutTransferBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  destLayoutTransferBarrier.image = readbackImage;
  destLayoutTransferBarrier.subresourceRange = layoutTransferRange;
  destLayoutTransferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  destLayoutTransferBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

  vkCmdPipelineBarrier(readbackCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0, 0, nullptr, 0, nullptr, 1, &layoutTransferBarrier);
  vkCmdPipelineBarrier(readbackCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
    0, 0, nullptr, 0, nullptr, 1, &srcLayoutTransferBarrier);
  vkCmdCopyImage(readbackCommandBuffer, renderData.rdSelectionImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    readbackImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);
  vkCmdPipelineBarrier(readbackCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
    0, 0, nullptr, 0, nullptr, 1, &destLayoutTransferBarrier);

  bool commandResult = CommandBuffer::submitSingleShotBuffer(renderData, renderData.rdCommandPool,
    readbackCommandBuffer, renderData.rdGraphicsQueue);

  if (!commandResult) {
    Logger::log(1, "%s error: could not submit readback transfer commands\n", __FUNCTION__);
    return pixelColor;
  }

  /* get image layout */
  VkImageSubresource subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkSubresourceLayout subResourceLayout{};

  vkGetImageSubresourceLayout(renderData.rdVkbDevice.device, readbackImage, &subResource, &subResourceLayout);

  /* map and read data */
  const float* data;
  result = vmaMapMemory(renderData.rdAllocator, readbackImageAlloc, (void**)&data);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map readback image memory (error: %i)\n", __FUNCTION__, result);
    return pixelColor;
  }

  data += yPos * subResourceLayout.rowPitch / sizeof(float) + xPos;
  pixelColor = *data;

  vmaUnmapMemory(renderData.rdAllocator, readbackImageAlloc);

  /* destroy local image, no longer needed */
  vmaDestroyImage(renderData.rdAllocator, readbackImage, readbackImageAlloc);

  return pixelColor;
}

void SelectionFramebuffer::cleanup(VkRenderData &renderData) {
  for (auto &fb : renderData.rdSelectionFramebuffers) {
    vkDestroyFramebuffer(renderData.rdVkbDevice.device, fb, nullptr);
  }
}
