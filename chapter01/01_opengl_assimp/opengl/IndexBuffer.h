/* Mininmal index buffer class */
#pragma once

#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "OGLRenderData.h"

class IndexBuffer {
  public:
    void init();
    void uploadData(std::vector<uint32_t> indices);
    void bind();
    void unbind();
    void cleanup();

  private:
    std::vector<unsigned int> mIndices {};
    GLuint mIndexVBO = 0;
};
