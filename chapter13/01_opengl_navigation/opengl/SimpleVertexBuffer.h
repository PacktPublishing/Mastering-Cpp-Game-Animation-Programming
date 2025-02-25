/* combined vertex and index buffer for Assimp... it's easier this way */
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "OGLRenderData.h"

class SimpleVertexBuffer {
public:
  void init();
  void uploadData(OGLLineMesh vertexData);

  void bind();
  void unbind();

  void draw();
  void bindAndDraw();

  void cleanup();

private:
  GLuint mVAO = 0;
  GLuint mVertexVBO = 0;

  size_t mNumVertices = 0;
};
