#include "SimpleVertexBuffer.h"
#include "Logger.h"

void SimpleVertexBuffer::init() {
  glGenVertexArrays(1, &mVAO);
  glGenBuffers(1, &mVertexVBO);

  glBindVertexArray(mVAO);

  glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OGLLineVertex), (void*) offsetof(OGLLineVertex, position));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(OGLLineVertex), (void*) offsetof(OGLLineVertex, color));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  Logger::log(1, "%s: VAO and VBOs initialized\n", __FUNCTION__);
}

void SimpleVertexBuffer::cleanup() {
  glDeleteBuffers(1, &mVertexVBO);
  glDeleteVertexArrays(1, &mVAO);
}

void SimpleVertexBuffer::uploadData(OGLLineMesh vertexData) {
  if (vertexData.vertices.size() == 0) {
    return;
  }

  mNumVertices = vertexData.vertices.size();

  glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, vertexData.vertices.size() * sizeof(OGLLineVertex), &vertexData.vertices.at(0), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SimpleVertexBuffer::bind() {
  glBindVertexArray(mVAO);
}

void SimpleVertexBuffer::unbind() {
  glBindVertexArray(0);
}

void SimpleVertexBuffer::draw() {
  glDrawArrays(GL_TRIANGLES, 0, mNumVertices);
}

void SimpleVertexBuffer::bindAndDraw() {
  bind();
  draw();
  unbind();
}

