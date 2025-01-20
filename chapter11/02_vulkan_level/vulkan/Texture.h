/* Vulkan texture */
#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include <assimp/texture.h>

#include "VkRenderData.h"

struct VkTextureStagingBuffer {
  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VmaAllocation stagingBufferAlloc = VK_NULL_HANDLE;
};

class Texture {
  public:
    static bool loadTexture(VkRenderData &renderData, VkTextureData &texData,  std::string textureFilename,
      bool generateMipmaps = true, bool flipImage = false);
    static bool loadTexture(VkRenderData &renderData, VkTextureData &texData,  std::string textureName,
      aiTexel* textureData, int width, int height, bool generateMipmaps = true, bool flipImage = false);

    static void cleanup(VkRenderData &renderData, VkTextureData &texData);

  private:
    static bool uploadToGPU(VkRenderData &renderData, VkTextureData &texData, VkTextureStagingBuffer &stagingData,
      int width, int height, bool generateMipmaps, int mipmapLevels);
};
