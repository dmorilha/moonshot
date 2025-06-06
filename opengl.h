#pragma once

#include <GLES2/gl2.h>

#include "types.h"

namespace opengl {
struct Framebuffer {
  struct Draw {
    ~Draw();
  private:
    Draw(const GLuint, const GLuint);
    Draw(Draw &) = delete;
    Draw(Draw &&) = delete;
    auto operator = (Draw &) -> Draw & = delete;
    auto operator = (Draw &&) -> Draw & = delete;
    friend class Framebuffer;
  };

  struct Read {
    ~Read();
  private:
    Read(const GLuint, const GLuint);
    Read(Read &) = delete;
    Read(Read &&) = delete;
    auto operator = (Read &) -> Read & = delete;
    auto operator = (Read &&) -> Read & = delete;
    friend class Framebuffer;
  };

  ~Framebuffer();
  Framebuffer() = default;
  Framebuffer(Framebuffer &&);
  Framebuffer & operator = (Framebuffer &&);

  auto bind() const -> void;
  auto draw() const -> Draw;
  auto read() const -> Read;

  static auto New(const GLsizei, const GLsizei, const Color & color = color::black) -> Framebuffer;

private:
  Framebuffer(Framebuffer &) = delete;
  Framebuffer & operator = (Framebuffer &) = delete;

  GLuint framebuffer_ = 0;
  GLuint texture_ = 0;
};

void clear(const GLsizei, const GLsizei, const Color & color = color::black);

} // end of namespace opengl
