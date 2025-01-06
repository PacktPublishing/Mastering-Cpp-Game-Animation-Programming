/* OpenGL shader stroage buffer */
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

#include "OGLRenderData.h"

class ShaderStorageBuffer {
  public:
    void init(size_t bufferSize);

    void uploadSsboData(std::vector<glm::mat4> bufferData, int bindingPoint);
    void uploadSsboData(std::vector<glm::mat2x4> bufferData, int bindingPoint);
    void uploadSsboData(std::vector<NodeTransformData> bufferData, int bindingPoint);
    void uploadSsboData(std::vector<int32_t> bufferData, int bindingPoint);
    void uploadSsboData(std::vector<glm::vec2> bufferData, int bindingPoint);

    /* just upload, use bind() call to use */
    void uploadSsboData(std::vector<glm::mat4> bufferData);
    void uploadSsboData(std::vector<int32_t> bufferData);

    void bind(int bindingPoint);
    GLuint getBufferId();

    void checkForResize(size_t newBufferSize);
    void cleanup();

  private:
    size_t mBufferSize = 0;
    GLuint mShaderStorageBuffer = 0;
};
