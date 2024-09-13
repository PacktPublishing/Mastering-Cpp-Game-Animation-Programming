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

std::vector<glm::mat4> ShaderStorageBuffer::getSsboDataMat4() {
  std::vector<glm::mat4> ssboData;
  ssboData.resize(mBufferSize / sizeof(glm::mat4));

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, mBufferSize, ssboData.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return ssboData;
}

std::vector<glm::mat4> ShaderStorageBuffer::getSsboDataMat4(int matricesOffset, int numberOfMatrices) {
  std::vector<glm::mat4> ssboData;
  ssboData.resize(numberOfMatrices);
  GLsizeiptr bufferSizeToRead = numberOfMatrices * sizeof(glm::mat4);
  GLintptr offset = matricesOffset * sizeof(glm::mat4);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, bufferSizeToRead, ssboData.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return ssboData;
}

std::vector<glm::vec4> ShaderStorageBuffer::getSsboDataVec4(int numberOfElements) {
  std::vector<glm::vec4> ssboData;
  ssboData.resize(numberOfElements);
  GLsizeiptr bufferSizeToRead = numberOfElements * sizeof(glm::vec4);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSizeToRead, ssboData.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return ssboData;
}

std::vector<int32_t> ShaderStorageBuffer::getSsboDataInt32() {
  std::vector<int32_t> ssboData;
  ssboData.resize(mBufferSize / sizeof(int32_t));

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, mBufferSize, ssboData.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return ssboData;
}

std::vector<AABB> ShaderStorageBuffer::getSsboDataAABB() {
  std::vector<AABB> ssboData;
  ssboData.resize(mBufferSize / sizeof(AABB));

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
  glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, mBufferSize, ssboData.data());
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  return ssboData;
}

void ShaderStorageBuffer::cleanup() {
  glDeleteBuffers(1, &mShaderStorageBuffer);
}
