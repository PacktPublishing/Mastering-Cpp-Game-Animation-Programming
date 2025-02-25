#include "LineVertexBuffer.h"
#include "Logger.h"

void LineVertexBuffer::init() {
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

  Logger::log(1, "%s: VAO and VBO initialized\n", __FUNCTION__);
}

void LineVertexBuffer::cleanup() {
  glDeleteBuffers(1, &mVertexVBO);
  glDeleteVertexArrays(1, &mVAO);
}

void LineVertexBuffer::uploadData(OGLLineMesh vertexData) {
  if (vertexData.vertices.empty()) {
    return;
  }

  glBindVertexArray(mVAO);
  glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);

  glBufferData(GL_ARRAY_BUFFER, vertexData.vertices.size() * sizeof(OGLLineVertex), &vertexData.vertices.at(0), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void LineVertexBuffer::bind() {
  glBindVertexArray(mVAO);
}

void LineVertexBuffer::unbind() {
  glBindVertexArray(0);
}

void LineVertexBuffer::draw(GLuint mode, unsigned int start, unsigned int num) {
  glDrawArrays(mode, start, num);
}

void LineVertexBuffer::bindAndDraw(GLuint mode, unsigned int start, unsigned int num) {
  bind();
  glDrawArrays(mode, start, num);
  unbind();
}

void LineVertexBuffer::bindAndDrawInstanced(GLuint mode, unsigned int start, unsigned int num, unsigned int instances) {
  bind();
  glDrawArraysInstanced(mode, start, num, instances);
  unbind();
}
