#pragma once
#include <vector>
#include <glm/glm.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OGLRenderData.h"

class SkyboxBuffer {
  public:
    void init();
    void uploadData(std::vector<OGLSkyboxVertex> vertexData);

    void bind();
    void unbind();

    void draw();

    void bindAndDraw();

    void cleanup();

  private:
    GLuint mVAO = 0;
    GLuint mVertexVBO = 0;

    int mNumVertices = 0;
};
