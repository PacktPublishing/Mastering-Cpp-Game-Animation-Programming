/* Mininmal index buffer class */
#pragma once
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OGLRenderData.h"

class IndexBuffer {
  public:
    void init();
    void uploadData(std::vector<uint32_t> indices);
  
    void bind();
    void unbind();
  
    void draw(GLuint mode, unsigned int start, unsigned int num);
    void drawIndirect(GLuint mode, unsigned int num);
    void drawIndirectInstanced(GLuint mode, unsigned int num, int instanceCount);
  
    void bindAndDraw(GLuint mode, unsigned int start, unsigned int num);
    void bindAndDrawIndirect(GLuint mode, unsigned int num);
    void bindAndDrawIndirectInstanced(GLuint mode, unsigned int num, int instanceCount);
  
    void cleanup();
  
  private:
    GLuint mVAO = 0;
    GLuint mIndexVBO = 0;
};
