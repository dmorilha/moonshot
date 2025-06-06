#include <iostream>

#include <cassert>

#include <GL/gl.h>

#include "opengl.h"

namespace opengl {

/* DRAW */
Framebuffer::Draw::~Draw() {
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
}

Framebuffer::Draw::Draw(const GLuint framebuffer, const GLuint texture) {
  assert(0 != framebuffer);
  assert(0 != texture);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
  glBindTexture(GL_TEXTURE_2D, texture);
}

/* READ */
Framebuffer::Read::~Read() {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
}

Framebuffer::Read::Read(const GLuint framebuffer, const GLuint texture) {
  assert(0 != framebuffer);
  assert(0 != texture);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
  glBindTexture(GL_TEXTURE_2D, texture);
}

Framebuffer::~Framebuffer() {
  if (0 != framebuffer_) {
    assert(0 != texture_);
    glDeleteTextures(1, &texture_);
    glDeleteFramebuffers(1, &framebuffer_);
  }
  framebuffer_ = texture_ = 0;
}

Framebuffer::Framebuffer(Framebuffer && other) {
  operator =(std::move(other));
}

Framebuffer & Framebuffer::operator = (Framebuffer && other) {
  std::swap(framebuffer_, other.framebuffer_);
  std::swap(texture_, other.texture_);
  return *this;
}

Framebuffer Framebuffer::New(const GLsizei width, const GLsizei height, const Color & color) {
  Framebuffer result;
  glGenFramebuffers(1, &(result.framebuffer_));
  assert(0 != result.framebuffer_);
  glGenTextures(1, &(result.texture_));
  assert(0 != result.texture_);
  glBindFramebuffer(GL_FRAMEBUFFER, result.framebuffer_);
  glBindTexture(GL_TEXTURE_2D, result.texture_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, result.texture_, 0);
  opengl::clear(width, height, color);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return result;
}

void Framebuffer::bind() const {
  assert(0 != framebuffer_);
  assert(0 != texture_);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  glBindTexture(GL_TEXTURE_2D, texture_);
}

Framebuffer::Draw Framebuffer::draw() const {
  return Framebuffer::Draw(framebuffer_, texture_);
}

Framebuffer::Read Framebuffer::read() const {
  return Framebuffer::Read(framebuffer_, texture_);
}

void clear(const GLsizei width, const GLsizei height, const Color & color) {
  glViewport(0, 0, width, height);
  glClearColor(color.red, color.green, color.blue, color.alpha);
  glClear(GL_COLOR_BUFFER_BIT);
}

} // end of namespace opengl
