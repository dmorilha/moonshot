#pragma once

#include <iostream>
#include <vector>

#include "char.h"
#include "freetype.h"
#include "wayland.h"

struct Dimensions {
  std::size_t column = 0;
  std::size_t glyphDescender = 0;
  std::size_t glyphHeight = 0;
  std::size_t glyphWidth = 0;
  std::size_t leftPadding = 10;
  std::size_t row = 0;
  std::size_t surfaceHeight = 0;
  std::size_t surfaceWidth = 0;
  std::size_t topPadding = 10;
  constexpr std::size_t columns() { return std::floor(surfaceWidth / glyphWidth); }
  constexpr std::size_t rows() { return std::floor(surfaceHeight / glyphHeight); }
  constexpr float scaleHeight() { return 2.f / surfaceHeight; }
  constexpr float scaleWidth() { return 2.f / surfaceWidth; }

  friend std::ostream & operator << (std::ostream &, const Dimensions &);

  Dimensions & operator = (const Dimensions &) = default;
};

struct Screen {
  using Buffer = std::vector<Char>;

  Screen() = delete;

  Screen(const Screen &) = delete;
  Screen(Screen &&);
  Screen & operator = (const Screen &) = delete;
  Screen & operator = (Screen && other) = delete;

  template <class ... Args>
  void loadFace(Args && ... args) {
    face_ = freetype_.load(std::forward<Args>(args)...);
    dimensions();
  }

  static Screen New(const wayland::Connection &);

  void write(const Buffer &);
  void clear();
  void makeCurrent() const;
  void paint();
  void repaint();
  bool swapBuffers() const;

private:
  Screen(std::unique_ptr<wayland::Surface> && surface) : surface_(std::move(surface)) { }

  void dimensions();

  freetype::Library freetype_ = {};

  freetype::Face face_ = {};
  std::unique_ptr<wayland::Surface> surface_;

  GLuint glProgram_ = 0;

  std::size_t cursor_left_ = 10;
  std::size_t cursor_top_ = 10;

  std::vector<Char> buffer_;
  bool repaint_ = false;

  Dimensions dimensions_;

  struct {
    GLint background = 0 /* = glGetUniformLocation(program, "background") */ ;
    GLint color = 0 /* = glGetUniformLocation(program, "color") */ ;
    GLint texture = 0 /* = glGetUniformLocation(program, "texture") */ ;
    GLint vpos = 0 /* = glGetAttribLocation(program, "vPos") */ ;
  } location_;
};
