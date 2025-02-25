#include "Framebuffer.h"
#include "Logger.h"

bool Framebuffer::init(VkRenderData &renderData) {
  renderData.rdSwapchainImages = renderData.rdVkbSwapchain.get_images().value();
  renderData.rdSwapchainImageViews = renderData.rdVkbSwapchain.get_image_views().value();

  renderData.rdFramebuffers.resize(renderData.rdSwapchainImageViews.size());

  for (unsigned int i = 0; i < renderData.rdSwapchainImageViews.size(); ++i) {
    std::vector<VkImageView> attachments = { renderData.rdSwapchainImageViews.at(i), renderData.rdDepthImageView };

    VkFramebufferCreateInfo FboInfo{};
    FboInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FboInfo.renderPass = renderData.rdRenderpass;
    FboInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    FboInfo.pAttachments = attachments.data();
    FboInfo.width = renderData.rdVkbSwapchain.extent.width;
    FboInfo.height = renderData.rdVkbSwapchain.extent.height;
    FboInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(renderData.rdVkbDevice.device, &FboInfo, nullptr, &renderData.rdFramebuffers.at(i));
    if (result != VK_SUCCESS) {
      Logger::log(1, "%s error: failed to create framebuffer %i (error: %i)\n", __FUNCTION__, i, result);
      return false;
    }
  }
  return true;
}

void Framebuffer::cleanup(VkRenderData &renderData) {
  for (auto &fb : renderData.rdFramebuffers) {
    vkDestroyFramebuffer(renderData.rdVkbDevice.device, fb, nullptr);
  }
}
