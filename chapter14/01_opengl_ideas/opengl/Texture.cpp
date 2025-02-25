#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <vector>

#include "Texture.h"
#include "Logger.h"

bool Texture::loadTexture(std::string textureFilename, bool flipImage) {
  mTextureName = textureFilename;

  stbi_set_flip_vertically_on_load(flipImage);
  /* always load as RGBA */
  unsigned char *textureData = stbi_load(textureFilename.c_str(), &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);

  if (!textureData) {
    Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, mTextureName.c_str());
    stbi_image_free(textureData);
    return false;
  }

  glGenTextures(1, &mTexture);
  glBindTexture(GL_TEXTURE_2D, mTexture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(textureData);

  Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, mTextureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
  return true;
}

bool Texture::loadTexture(std::string textureName, aiTexel* textureData, int width, int height, bool flipImage) {
  if (!textureData) {
    Logger::log(1, "%s error: could not load texture '%s'\n", __FUNCTION__, textureName.c_str());
    return false;
  }

  Logger::log(1, "%s: texture file '%s' has width %i and height %i\n", __FUNCTION__, textureName.c_str(), width, height);

  glGenTextures(1, &mTexture);
  glBindTexture(GL_TEXTURE_2D, mTexture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  /* allow to flip the image, similar to file loaded from disk */
  stbi_set_flip_vertically_on_load(flipImage);

  /* we use stbi to detect the in-memory format, but always request RGBA */
  unsigned char *data = nullptr;
  if (height == 0)   {
    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width, &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);
  }
  else   {
    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width * height, &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

  stbi_image_free(data);

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, textureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
  return true;
}

bool Texture::loadCubemapTexture(std::string textureFilename, bool flipImage) {
  mTextureName = textureFilename;

  stbi_set_flip_vertically_on_load(flipImage);
  /* always convert to RGBA */
  unsigned char *textureData = stbi_load(textureFilename.c_str(), &mTexWidth, &mTexHeight, &mNumberOfChannels, STBI_rgb_alpha);

  if (!textureData) {
    Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, mTextureName.c_str());
    stbi_image_free(textureData);
    return false;
  }

  glGenTextures(1, &mTexture);
  glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture);

  /* no mip mapping */
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

  glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

  /*
   * Cube Map is a cross, so single image width is 1/4, height is 1/3
   * 0,0
   *  +----+----+----+----+
   *  |    | +Y |    |    |
   *  +----+----+----+----+
   *  | -X | -Z | +X | +Z |
   *  +----+----+----+----+
   *  |    | -Y |    |    |
   *  +----+----+----+----+ w
   *                      h
   *
   * GL_TEXTURE_CUBE_MAP_POSITIVE_X -> Right
   * GL_TEXTURE_CUBE_MAP_NEGATIVE_X -> Left
   * GL_TEXTURE_CUBE_MAP_POSITIVE_Y -> Top
   * GL_TEXTURE_CUBE_MAP_NEGATIVE_Y -> Bottom
   * GL_TEXTURE_CUBE_MAP_POSITIVE_Z -> Back
   * GL_TEXTURE_CUBE_MAP_NEGATIVE_Z -> Front
   */

  int cubeFaceWidth = mTexWidth / 4.0;
  int cubeFaceHeight = mTexHeight / 3.0;
  int cubeFacePositions[][2] = {
    { cubeFaceWidth * 2, cubeFaceHeight }, // right, +X
    { 0, cubeFaceHeight}, // left, -X
    { cubeFaceWidth, 0 }, // top, +Y
    { cubeFaceWidth, cubeFaceHeight * 2 }, // bottom, -Y
    // inverted z axis?!
    { cubeFaceWidth, cubeFaceHeight }, // front, -Z
    { cubeFaceWidth * 3, cubeFaceHeight }, // back, +Z
  };

  std::vector<unsigned char> subImage{};
  subImage.resize(cubeFaceHeight * cubeFaceWidth * 4);

  for (int face = 0; face < 6; ++face) {
    for (int i = 0; i < cubeFaceHeight; ++i) {
      std::memcpy(subImage.data() + i * cubeFaceWidth * 4, textureData + cubeFacePositions[face][0] * 4  + (cubeFacePositions[face][1] + i) * mTexWidth * 4, cubeFaceWidth * 4);
    }
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_SRGB8_ALPHA8, cubeFaceWidth, cubeFaceHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, subImage.data());
  }

  // nearest filter and clamp to edge for Cube Map
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  stbi_image_free(textureData);

  Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, mTextureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
  return true;
}

void Texture::cleanup() {
  glDeleteTextures(1, &mTexture);
}

void Texture::bind() {
  glBindTexture(GL_TEXTURE_2D, mTexture);
}

void Texture::bindCubemap() {
  glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture);
}

void Texture::unbind() {
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::unbindCubemap() {
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

