#include "IndexBuffer.h"

#include "Logger.h"

void IndexBuffer::init() {
  glGenBuffers(1, &mIndexVBO);

  Logger::log(1, "%s: index buffer created\n", __FUNCTION__);
}

void IndexBuffer::bind() {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
}

void IndexBuffer::unbind() {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::uploadData(std::vector<uint32_t> indices) {
  if (indices.size() == 0) {
    return;
  }

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexVBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices.at(0), GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::cleanup() {
  glDeleteBuffers(1, &mIndexVBO);
}
