#include "Framebuffer.h"
#include "Logger.h"

bool Framebuffer::init(unsigned int width, unsigned int height) {
  mBufferWidth = width;
  mBufferHeight = height;

  glGenFramebuffers(1, &mBuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, mBuffer);

  /* color texture */
  glGenTextures(1, &mColorTex);
  glBindTexture(GL_TEXTURE_2D, mColorTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mColorTex, 0);
  Logger::log(1, "%s: added color buffer\n", __FUNCTION__);

  /* selection texture */
  glGenTextures(1, &mSelectionTex);
  glBindTexture(GL_TEXTURE_2D, mSelectionTex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, nullptr);

  glBindTexture(GL_TEXTURE_2D, 0);

  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, mSelectionTex, 0);
  Logger::log(1, "%s: added selection buffer\n", __FUNCTION__);

  const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
  glDrawBuffers(2, buffers);
  Logger::log(1, "%s: drawing to color and selection buffer\n", __FUNCTION__);

  /* render buffer as depth buffer */
  glGenRenderbuffers(1, &mDepthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, mDepthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  Logger::log(1, "%s: added depth renderbuffer\n", __FUNCTION__);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return checkComplete();
}

void Framebuffer::cleanup() {
  unbind();

  glDeleteTextures(1, &mSelectionTex);
  glDeleteTextures(1, &mColorTex);
  glDeleteRenderbuffers(1, &mDepthBuffer);
  glDeleteFramebuffers(1, &mBuffer);
}

bool Framebuffer::resize(unsigned int newWidth, unsigned int newHeight) {
  Logger::log(1, "%s: resizing framebuffer from %dx%d to %dx%d\n", __FUNCTION__, mBufferWidth, mBufferHeight, newWidth, newHeight);
  mBufferWidth = newWidth;
  mBufferHeight = newHeight;

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glDeleteTextures(1, &mSelectionTex);
  glDeleteTextures(1, &mColorTex);
  glDeleteRenderbuffers(1, &mDepthBuffer);
  glDeleteFramebuffers(1, &mBuffer);

  return init(newWidth, newHeight);
}

void Framebuffer::bind() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mBuffer);
}

void Framebuffer::unbind() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void Framebuffer::drawToScreen() {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, mBuffer);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, mBufferWidth, mBufferHeight, 0, 0, mBufferWidth, mBufferHeight,
                  GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

bool Framebuffer::checkComplete() {
  glBindFramebuffer(GL_FRAMEBUFFER, mBuffer);

  GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (result != GL_FRAMEBUFFER_COMPLETE) {
    Logger::log(1, "%s error: framebuffer is NOT complete\n", __FUNCTION__);
    return false;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  Logger::log(1, "%s: framebuffer is complete\n", __FUNCTION__);
  return true;
}

void Framebuffer::clearTextures() {
  static GLfloat colorClear[] = { 0.25f, 0.25f, 0.25f, 1.0f };
  glClearBufferfv(GL_COLOR, 0, colorClear);

  static GLfloat selectionClearColor = -1.0f;
  glClearBufferfv(GL_COLOR, 1, &selectionClearColor);

  static GLfloat depthValue = 1.0f;
  glClearBufferfv(GL_DEPTH, 0, &depthValue);
}

float Framebuffer::readPixelFromPos(unsigned int xPos, unsigned int yPos) {
  /* random default value to detect errors */
  float pixelColor = -444.0f;

  glBindFramebuffer(GL_READ_FRAMEBUFFER, mBuffer);
  glReadBuffer(GL_COLOR_ATTACHMENT1);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glReadPixels(xPos, yPos, 1, 1, GL_RED, GL_FLOAT, &pixelColor);

  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  return pixelColor;
}
