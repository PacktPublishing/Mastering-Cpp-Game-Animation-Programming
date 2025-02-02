/* OpenGL shader stroage buffer */
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "OGLRenderData.h"
#include "AABB.h"
#include "Logger.h"

class ShaderStorageBuffer {
  public:
    void init(size_t bufferSize);

    /* upload and bind */
    template <typename T>
    void uploadSsboData(std::vector<T> bufferData, int bindingPoint) {
      if (bufferData.empty()) {
        return;
      }

      size_t bufferSize = bufferData.size() * sizeof(T);
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

    template <typename T>
    void uploadSsboData(T bufferData, int bindingPoint) {
      size_t bufferSize =  sizeof(T);
      if (bufferSize > mBufferSize) {
        Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
        cleanup();
        init(bufferSize);
      }

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, &bufferData);
      glBindBufferRange(GL_SHADER_STORAGE_BUFFER, bindingPoint, mShaderStorageBuffer, 0,
        bufferSize);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    /* just upload, use bind() call to use */
    template <typename T>
    void uploadSsboData(std::vector<T> bufferData) {
      if (bufferData.empty()) {
        return;
      }

      size_t bufferSize = bufferData.size() * sizeof(T);
      if (bufferSize > mBufferSize) {
        Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
        cleanup();
        init(bufferSize);
      }

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData.data());
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    template <typename T>
    void uploadSsboData(T bufferData) {
      size_t bufferSize = sizeof(T);
      if (bufferSize > mBufferSize) {
        Logger::log(1, "%s: resizing SSBO %i from %i to %i bytes\n", __FUNCTION__, mShaderStorageBuffer, mBufferSize, bufferSize);
        cleanup();
        init(bufferSize);
      }

      glBindBuffer(GL_SHADER_STORAGE_BUFFER, mShaderStorageBuffer);
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bufferSize, bufferData);
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void bind(int bindingPoint);
    GLuint getBufferId();

    std::vector<glm::mat4> getSsboDataMat4();
    std::vector<glm::mat4> getSsboDataMat4(int matricesOffset, int numberOfMatrices);
    std::vector<glm::vec4> getSsboDataVec4(int numberOfElements);

    void checkForResize(size_t newBufferSize);
    void cleanup();

  private:
    size_t mBufferSize = 0;
    GLuint mShaderStorageBuffer = 0;
};
