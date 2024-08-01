#include "VertexIndexBuffer.h"
#include "Logger.h"

void VertexIndexBuffer::init() {
  glGenVertexArrays(1, &mVAO);
  glGenBuffers(1, &mVertexVBO);
  glGenBuffers(1, &mIndexVBO);

  glBindVertexArray(mVAO);

  glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);

  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, position));
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, color));
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, normal));
  glVertexAttribIPointer(3, 4, GL_UNSIGNED_INT,   sizeof(OGLVertex), (void*) offsetof(OGLVertex, boneNumber));
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(OGLVertex), (void*) offsetof(OGLVertex, boneWeight));

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glEnableVertexAttribArray(3);
  glEnableVertexAttribArray(4);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
  /* do NOT unbind index buffer here!*/

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  Logger::log(1, "%s: VAO and VBOs initialized\n", __FUNCTION__);
}

void VertexIndexBuffer::cleanup() {
  glDeleteBuffers(1, &mIndexVBO);
  glDeleteBuffers(1, &mVertexVBO);
  glDeleteVertexArrays(1, &mVAO);
}

void VertexIndexBuffer::uploadData(std::vector<OGLVertex> vertexData, std::vector<uint32_t> indices) {
  if (vertexData.size() == 0 || indices.size() == 0) {
    Logger::log(1, "%s error: invalid data to upload (vertices: %i, indices: %i)\n", __FUNCTION__, vertexData.size(), indices.size());
    return;
  }

  glBindBuffer(GL_ARRAY_BUFFER, mVertexVBO);
  glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(OGLVertex), &vertexData.at(0), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices.at(0), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void VertexIndexBuffer::bind() {
  glBindVertexArray(mVAO);
}

void VertexIndexBuffer::unbind() {
  glBindVertexArray(0);
}

void VertexIndexBuffer::draw(GLuint mode, unsigned int start, unsigned int num) {
  glDrawArrays(mode, start, num);
}

void VertexIndexBuffer::bindAndDraw(GLuint mode, unsigned int start, unsigned int num) {
  bind();
  draw(mode, start, num);
  unbind();
}

void VertexIndexBuffer::drawIndirect(GLuint mode, unsigned int num) {
  glDrawElements(mode, num, GL_UNSIGNED_INT, 0);
}

void VertexIndexBuffer::bindAndDrawIndirect(GLuint mode, unsigned int num) {
  bind();
  drawIndirect(mode, num);
  unbind();
}

void VertexIndexBuffer::drawIndirectInstanced(GLuint mode, unsigned int num, int instanceCount) {
  glDrawElementsInstanced(mode, num, GL_UNSIGNED_INT, 0, instanceCount);
}

void VertexIndexBuffer::bindAndDrawIndirectInstanced(GLuint mode, unsigned int num, int instanceCount) {
  bind();
  drawIndirectInstanced(mode, num, instanceCount);
  unbind();
}
