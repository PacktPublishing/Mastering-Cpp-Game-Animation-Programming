#include "SelectionRenderpass.h"
#include "Logger.h"

#include <VkBootstrap.h>

bool SelectionRenderpass::init(VkRenderData &renderData) {
  VkAttachmentDescription colorAtt{};
  colorAtt.format = renderData.rdVkbSwapchain.image_format;
  colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  /* load previous image */
  colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  /* must be previous image format */
  colorAtt.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttRef{};
  colorAttRef.attachment = 0;
  colorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  /* separate selection buffer */
  VkAttachmentDescription selectionColorAtt{};
  selectionColorAtt.format = renderData.rdSelectionFormat;
  selectionColorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  selectionColorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  selectionColorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  selectionColorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  selectionColorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  selectionColorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  selectionColorAtt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference selectionColorAttRef{};
  selectionColorAttRef.attachment = 1;
  selectionColorAttRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAtt{};
  depthAtt.flags = 0;
  depthAtt.format = renderData.rdDepthFormat;
  depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAtt.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttRef{};
  depthAttRef.attachment = 2;
  depthAttRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  std::vector<VkAttachmentReference> attachmentRefs = { colorAttRef, selectionColorAttRef };

  VkSubpassDescription subpassDesc{};
  subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDesc.colorAttachmentCount = static_cast<uint32_t>(attachmentRefs.size());
  subpassDesc.pColorAttachments = attachmentRefs.data();
  subpassDesc.pDepthStencilAttachment = &depthAttRef;

  VkSubpassDependency subpassDep{};
  subpassDep.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDep.dstSubpass = 0;
  subpassDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDep.srcAccessMask = 0;
  subpassDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDependency depthDep{};
  depthDep.srcSubpass = VK_SUBPASS_EXTERNAL;
  depthDep.dstSubpass = 0;
  depthDep.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depthDep.srcAccessMask = 0;
  depthDep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depthDep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::vector<VkSubpassDependency> dependencies = { subpassDep, depthDep };
  std::vector<VkAttachmentDescription> attachments = { colorAtt, selectionColorAtt, depthAtt };

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDesc;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();

  VkResult result = vkCreateRenderPass(renderData.rdVkbDevice.device, &renderPassInfo, nullptr, &renderData.rdSelectionRenderpass);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error; could not create selection renderpass (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  return true;
}

void SelectionRenderpass::cleanup(VkRenderData &renderData) {
  vkDestroyRenderPass(renderData.rdVkbDevice.device, renderData.rdSelectionRenderpass, nullptr);
}
