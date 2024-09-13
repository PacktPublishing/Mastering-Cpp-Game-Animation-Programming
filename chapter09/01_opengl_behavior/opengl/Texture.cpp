#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "Texture.h"
#include "Logger.h"

void Texture::cleanup() {
  glDeleteTextures(1, &mTexture);
}

bool Texture::loadTexture(std::string textureFilename, bool flipImage) {
  mTextureName = textureFilename;

  stbi_set_flip_vertically_on_load(flipImage);
  unsigned char *textureData = stbi_load(textureFilename.c_str(), &mTexWidth, &mTexHeight, &mNumberOfChannels, 0);

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

  switch(mNumberOfChannels) {
    case 3:
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, mTexWidth, mTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
      break;
    case 4:
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
      break;
    default:
      Logger::log(1, "%s error: image has %i channels, supported are only 3 (RGB) or 4 (RGBA)\n", __FUNCTION__, mNumberOfChannels);
      break;
  }

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

  /* we must use stbi to detect the in-memory format */
  unsigned char *data = nullptr;
  if (height == 0)   {
    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width, &mTexWidth, &mTexHeight, &mNumberOfChannels, 0);
  }
  else   {
    data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(textureData), width * height, &mTexWidth, &mTexHeight, &mNumberOfChannels, 0);
  }

  switch(mNumberOfChannels) {
    case 3:
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, mTexWidth, mTexHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      break;
    case 4:
      glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, mTexWidth, mTexHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
      break;
    default:
      Logger::log(1, "%s error: image has %i channels, supported are only 3 (RGB) or 4 (RGBA)\n", __FUNCTION__, mNumberOfChannels);
      break;
  }

  stbi_image_free(data);

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  Logger::log(1, "%s: texture '%s' loaded (%dx%d, %d channels)\n", __FUNCTION__, textureName.c_str(), mTexWidth, mTexHeight, mNumberOfChannels);
  return true;
}


void Texture::bind() {
  glBindTexture(GL_TEXTURE_2D, mTexture);
}

void Texture::unbind() {
  glBindTexture(GL_TEXTURE_2D, 0);
}
