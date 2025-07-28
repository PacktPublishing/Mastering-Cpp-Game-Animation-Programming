#include <cstring>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "CommandBuffer.h"
#include "Texture.h"
#include "Logger.h"

bool Texture::loadTexture(VkRenderData &renderData, VkTextureData &texData, std::string textureFilename,
      bool generateMipmaps, bool flipImage) {
  int texWidth;
  int texHeight;
  int numberOfChannels;
  uint32_t mipmapLevels = 1;

  stbi_set_flip_vertically_on_load(flipImage);
  /* always load as RGBA */
  unsigned char *textureData = stbi_load(textureFilename.c_str(), &texWidth, &texHeight, &numberOfChannels, STBI_rgb_alpha);

  if (!textureData) {
    Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, textureFilename.c_str());
    stbi_image_free(textureData);
    return false;
  }

  if (generateMipmaps) {
    mipmapLevels += static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight))));
  }

  VkDeviceSize imageSize = texWidth * texHeight * 4;

  /* staging buffer */
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size = imageSize;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VkBuffer stagingBuffer;
  VmaAllocation stagingBufferAlloc;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  VkResult result = vmaCreateBuffer(renderData.rdAllocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer,  &stagingBufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate texture staging buffer via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  void* uploadData;
  result = vmaMapMemory(renderData.rdAllocator, stagingBufferAlloc, &uploadData);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map texture memory (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  std::memcpy(uploadData, textureData, imageSize);
  vmaUnmapMemory(renderData.rdAllocator, stagingBufferAlloc);
  vmaFlushAllocation(renderData.rdAllocator, stagingBufferAlloc, 0, imageSize);

  stbi_image_free(textureData);

  VkTextureStagingBuffer stagingData = { stagingBuffer, stagingBufferAlloc };

  if (!uploadToGPU(renderData, texData, stagingData, texWidth, texHeight, generateMipmaps, mipmapLevels)) {
    Logger::log(1, "%s error: could not load texture '%s'\n", __FUNCTION__, textureFilename.c_str());
    return false;
  }

  Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, textureFilename.c_str(), texWidth, texHeight, numberOfChannels);
  return true;
}

bool Texture::loadTexture(VkRenderData& renderData, VkTextureData& texData, std::string textureName, aiTexel* textureData, int width, int height, bool generateMipmaps, bool flipImage) {
  if (!textureData) {
    Logger::log(1, "%s error: could not load texture '%s'\n", __FUNCTION__, textureName.c_str());
    return false;
  }

  int texWidth;
  int texHeight;
  int numberOfChannels;
  uint32_t mipmapLevels = 1;

  /* allow to flip the image, similar to file loaded from disk */
  stbi_set_flip_vertically_on_load(flipImage);

  /* we use stbi to detect the in-memory format, but always request RGBA */
  unsigned char *data = nullptr;
  if (height == 0)   {
    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width, &texWidth, &texHeight, &numberOfChannels, STBI_rgb_alpha);
  }
  else   {
    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width * height, &texWidth, &texHeight, &numberOfChannels, STBI_rgb_alpha);
  }

  if (!data) {
    Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, textureName.c_str());
    stbi_image_free(data);
    return false;
  }

  if (generateMipmaps) {
    mipmapLevels += static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight))));
  }

  VkDeviceSize imageSize = texWidth * texHeight * 4;

  /* staging buffer */
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size = imageSize;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VkBuffer stagingBuffer;
  VmaAllocation stagingBufferAlloc;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

  VkResult result = vmaCreateBuffer(renderData.rdAllocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer,  &stagingBufferAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate texture staging buffer via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  void* uploadData;
  result = vmaMapMemory(renderData.rdAllocator, stagingBufferAlloc, &uploadData);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not map texture memory (error: %i)\n", __FUNCTION__, result);
    return false;
  }
  std::memcpy(uploadData, data, imageSize);
  vmaUnmapMemory(renderData.rdAllocator, stagingBufferAlloc);
  vmaFlushAllocation(renderData.rdAllocator, stagingBufferAlloc, 0, imageSize);

  stbi_image_free(data);

  VkTextureStagingBuffer stagingData = { stagingBuffer, stagingBufferAlloc };
  if (!uploadToGPU(renderData, texData, stagingData, texWidth, texHeight, generateMipmaps, mipmapLevels)) {
    Logger::log(1, "%s error: could not load texture '%s'\n", __FUNCTION__, textureName.c_str());
    return false;
  }

  Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, textureName.c_str(), texWidth, texHeight, numberOfChannels);
  return true;
}

bool Texture::uploadToGPU(VkRenderData& renderData, VkTextureData& texData, VkTextureStagingBuffer &stagingData,
      uint32_t width, uint32_t height, bool generateMipmaps, uint32_t mipmapLevels) {
  /* upload */
  VkCommandBuffer uploadCommandBuffer = CommandBuffer::createSingleShotBuffer(renderData);

  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = mipmapLevels;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  VmaAllocationCreateInfo imageAllocInfo{};
  imageAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  VkResult result = vmaCreateImage(renderData.rdAllocator, &imageInfo, &imageAllocInfo, &texData.image,  &texData.imageAlloc, nullptr);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate texture image via VMA (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkImageSubresourceRange stagingBufferRange{};
  stagingBufferRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  stagingBufferRange.baseMipLevel = 0;
  stagingBufferRange.levelCount = mipmapLevels;
  stagingBufferRange.baseArrayLayer = 0;
  stagingBufferRange.layerCount = 1;

  /* 1st barrier, undefined to transfer optimal */
  VkImageMemoryBarrier stagingBufferTransferBarrier{};
  stagingBufferTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  stagingBufferTransferBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  stagingBufferTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  stagingBufferTransferBarrier.image = texData.image;
  stagingBufferTransferBarrier.subresourceRange = stagingBufferRange;
  stagingBufferTransferBarrier.srcAccessMask = 0;
  stagingBufferTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  VkExtent3D textureExtent{};
  textureExtent.width = width;
  textureExtent.height = height;
  textureExtent.depth = 1;

  VkBufferImageCopy stagingBufferCopy{};
  stagingBufferCopy.bufferOffset = 0;
  stagingBufferCopy.bufferRowLength = 0;
  stagingBufferCopy.bufferImageHeight = 0;
  stagingBufferCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  stagingBufferCopy.imageSubresource.mipLevel = 0;
  stagingBufferCopy.imageSubresource.baseArrayLayer = 0;
  stagingBufferCopy.imageSubresource.layerCount = 1;
  stagingBufferCopy.imageExtent = textureExtent;

  /* 2nd barrier, transfer optimal to shader optimal */
  VkImageMemoryBarrier stagingBufferShaderBarrier{};
  stagingBufferShaderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  stagingBufferShaderBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  if (mipmapLevels > 1) {
    /* VkCmdBlit() requires the original image to be in TRANSFER_DST_OPTIMAL format */
    stagingBufferShaderBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  } else {
    stagingBufferShaderBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }
  stagingBufferShaderBarrier.image = texData.image;
  stagingBufferShaderBarrier.subresourceRange = stagingBufferRange;
  stagingBufferShaderBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  stagingBufferShaderBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(uploadCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &stagingBufferTransferBarrier);
  vkCmdCopyBufferToImage(uploadCommandBuffer, stagingData.stagingBuffer, texData.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &stagingBufferCopy);
  vkCmdPipelineBarrier(uploadCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &stagingBufferShaderBarrier);

  /* generate mipmap blit commands */
  if (generateMipmaps) {
    VkImageSubresourceRange blitRange{};
    blitRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRange.baseMipLevel = 0;
    blitRange.levelCount = 1;
    blitRange.baseArrayLayer = 0;
    blitRange.layerCount = 1;

    /* 1st barrier, we need to transfer to src optimal for the blit */
    VkImageMemoryBarrier firstBarrier{};
    firstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    firstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    firstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    firstBarrier.image = texData.image;
    firstBarrier.subresourceRange = blitRange;
    firstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    firstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    /* 2nd barrier -> transfer to shader optimal */
    VkImageMemoryBarrier secondBarrier{};
    secondBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    secondBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    secondBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    secondBarrier.image = texData.image;
    secondBarrier.subresourceRange = blitRange;
    secondBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    secondBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    VkImageBlit mipBlit{};
    mipBlit.srcOffsets[0] = { 0, 0, 0 };
    mipBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mipBlit.srcSubresource.baseArrayLayer = 0;
    mipBlit.srcSubresource.layerCount = 1;

    mipBlit.dstOffsets[0] = { 0, 0, 0 };
    mipBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mipBlit.dstSubresource.baseArrayLayer = 0;
    mipBlit.dstSubresource.layerCount = 1;

    int32_t mipWidth = width;
    int32_t mipHeight = height;
    for (int i = 1; i < mipmapLevels; ++i) {
      mipBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
      mipBlit.srcSubresource.mipLevel = i - 1;

      mipBlit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
      mipBlit.dstSubresource.mipLevel = i;

      firstBarrier.subresourceRange.baseMipLevel = i -1;
      vkCmdPipelineBarrier(uploadCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &firstBarrier);

      vkCmdBlitImage(uploadCommandBuffer,
                     texData.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     texData.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1, &mipBlit, VK_FILTER_LINEAR);

      secondBarrier.subresourceRange.baseMipLevel = i -1;
      vkCmdPipelineBarrier(uploadCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &secondBarrier);

      if (mipWidth > 1) {
        mipWidth /= 2;
      }
      if (mipHeight > 1) {
        mipHeight /= 2;
      }

      Logger::log(1, "%s: created level %i with width %i and height %i\n", __FUNCTION__, i, mipWidth, mipHeight);
    }

    if (mipmapLevels > 1) {
      VkImageMemoryBarrier lastBarrier{};
      lastBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      lastBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      lastBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      lastBarrier.image = texData.image;
      lastBarrier.subresourceRange = blitRange;
      lastBarrier.subresourceRange.baseMipLevel = mipmapLevels - 1;
      lastBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      lastBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

      vkCmdPipelineBarrier(uploadCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                           0, 0, nullptr, 0, nullptr, 1, &lastBarrier);
    }
  }

  bool commandResult = CommandBuffer::submitSingleShotBuffer(renderData, uploadCommandBuffer, renderData.rdGraphicsQueue);
  vmaDestroyBuffer(renderData.rdAllocator, stagingData.stagingBuffer, stagingData.stagingBufferAlloc);

  if (!commandResult) {
    Logger::log(1, "%s error: could not submit texture transfer commands\n", __FUNCTION__);
    return false;
  }

  /* image view and sampler */
  VkImageViewCreateInfo texViewInfo{};
  texViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  texViewInfo.image = texData.image;
  texViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  texViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
  texViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  texViewInfo.subresourceRange.baseMipLevel = 0;
  texViewInfo.subresourceRange.levelCount = mipmapLevels;
  texViewInfo.subresourceRange.baseArrayLayer = 0;
  texViewInfo.subresourceRange.layerCount = 1;

  result = vkCreateImageView(renderData.rdVkbDevice.device, &texViewInfo, nullptr, &texData.imageView);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create image view for texture\n", __FUNCTION__);
    return false;
  }

  VkPhysicalDeviceFeatures supportedFeatures{};
  vkGetPhysicalDeviceFeatures(renderData.rdVkbPhysicalDevice.physical_device, &supportedFeatures);
  Logger::log(2, "%s: anisotropy supported: %s\n", __FUNCTION__, supportedFeatures.samplerAnisotropy ? "yes" : "no");
  const VkBool32 anisotropyAvailable = supportedFeatures.samplerAnisotropy;

  VkPhysicalDeviceProperties physProperties{};
  vkGetPhysicalDeviceProperties(renderData.rdVkbPhysicalDevice.physical_device, &physProperties);
  const float maxAnisotropy = physProperties.limits.maxSamplerAnisotropy;
  Logger::log(2, "%s: device supports max anisotropy of %f\n", __FUNCTION__, maxAnisotropy);

  VkSamplerCreateInfo texSamplerInfo{};
  texSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  texSamplerInfo.magFilter = VK_FILTER_LINEAR;
  texSamplerInfo.minFilter = VK_FILTER_LINEAR;
  texSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  texSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  texSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  texSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  texSamplerInfo.unnormalizedCoordinates = VK_FALSE;
  texSamplerInfo.compareEnable = VK_FALSE;
  texSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  texSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  texSamplerInfo.mipLodBias = 0.0f;
  texSamplerInfo.minLod = 0.0f;
  texSamplerInfo.maxLod = static_cast<float>(mipmapLevels);
  texSamplerInfo.anisotropyEnable = anisotropyAvailable;
  texSamplerInfo.maxAnisotropy = maxAnisotropy;

  result = vkCreateSampler(renderData.rdVkbDevice.device, &texSamplerInfo, nullptr, &texData.sampler);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not create sampler for texture (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkDescriptorSetAllocateInfo descriptorAllocateInfo{};
  descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorAllocateInfo.descriptorPool = renderData.rdDescriptorPool;
  descriptorAllocateInfo.descriptorSetCount = 1;
  descriptorAllocateInfo.pSetLayouts = &renderData.rdAssimpTextureDescriptorLayout;

  result = vkAllocateDescriptorSets(renderData.rdVkbDevice.device, &descriptorAllocateInfo, &texData.descriptorSet);
  if (result != VK_SUCCESS) {
    Logger::log(1, "%s error: could not allocate descriptor set (error: %i)\n", __FUNCTION__, result);
    return false;
  }

  VkDescriptorImageInfo descriptorImageInfo{};
  descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptorImageInfo.imageView = texData.imageView;
  descriptorImageInfo.sampler = texData.sampler;

  VkWriteDescriptorSet writeDescriptorSet{};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet.dstSet = texData.descriptorSet;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.pImageInfo = &descriptorImageInfo;

  vkUpdateDescriptorSets(renderData.rdVkbDevice.device, 1, &writeDescriptorSet, 0, nullptr);

  return true;
}

void Texture::cleanup(VkRenderData &renderData, VkTextureData &texData) {
  vkFreeDescriptorSets(renderData.rdVkbDevice.device, renderData.rdDescriptorPool, 1, &texData.descriptorSet);
  vkDestroySampler(renderData.rdVkbDevice.device, texData.sampler, nullptr);
  vkDestroyImageView(renderData.rdVkbDevice.device, texData.imageView, nullptr);
  vmaDestroyImage(renderData.rdAllocator, texData.image, texData.imageAlloc);
}
