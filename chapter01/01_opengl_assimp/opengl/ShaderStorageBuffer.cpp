#include "ShaderStorageBuffer.h"
#include "Logger.h"

void ShaderStorageBuffer::init(size_t bufferSize) {
  mBufferSize = bufferSize;

  glGenBuffers(1, &mShaderStorageBuffer);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glBufferData(GL_SHADER_STORAGE_BUFFER, mBufferSize, nullptr, GL_DYNAMIC_COPY);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::uploadSsboData(std::vector<glm::mat4> bufferData, int bindingPoint) {
  if (bufferData.empty()) {
    return;
  }

  size_t bufferSize = bufferData.size() * sizeof(glm::mat4);
  if (bufferSize > mBufferSize) {
    Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
    cleanup();
    init(bufferSize);
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, mShaderStorageBuffer, 0,
    bufferSize);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::uploadSsboData(std::vector<glm::mat2x4> bufferData, int bindingPoint) {
  if (bufferData.empty()) {
    return;
  }

  size_t bufferSize = bufferData.size() * sizeof(glm::mat2x4);
  if (bufferSize > mBufferSize) {
    Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
    cleanup();
    init(bufferSize);
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
  glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, mShaderStorageBuffer, 0,
    bufferSize);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void ShaderStorageBuffer::cleanup() {
  glDeleteBuffers(1, &mShaderStorageBuffer);
}
