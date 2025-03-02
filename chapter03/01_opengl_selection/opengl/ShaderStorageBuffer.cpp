#include "ShaderStorageBuffer.h"

void ShaderStorageBuffer::init(size_t bufferSize) {
  mBufferSize = bufferSize;

  glGenBuffers(1, &mShaderStorageBuffer);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, mBufferSize, nullptr, GL_DYNAMIC_COPY);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::bind(int bindingPoint) {
  if (mBufferSize == 0) {
    return;
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, mShaderStorageBuffer);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GLuint ShaderStorageBuffer::getBufferId() {
  return mShaderStorageBuffer;
}

void ShaderStorageBuffer::checkForResize(size_t newBufferSize) {
  if (newBufferSize > mBufferSize) {
    Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, newBufferSize);
    cleanup();
    init(newBufferSize);
  }
}

void ShaderStorageBuffer::cleanup() {
  glDeleteBuffers(1, &mShaderStorageBuffer);
}
