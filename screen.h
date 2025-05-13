#pragma once

#include <iostream>
#include <vector>

#include "buffer.h"
#include "freetype.h"
#include "wayland.h"

struct Dimensions {
  std::size_t column = 0;
  std::size_t cursor_left = 10;
  std::size_t cursor_top = 10;
  std::size_t glyphDescender = 0;
  std::size_t glyphHeight = 0;
  std::size_t glyphWidth = 0;
  std::size_t leftPadding = 10;
  std::size_t row = 0;
  std::size_t surfaceHeight = 0;
  std::size_t surfaceWidth = 0;
  std::size_t topPadding = 10;
  constexpr std::size_t columns() const { return std::floor(surfaceWidth / glyphWidth); }
  constexpr std::size_t rows() const { return std::floor(surfaceHeight / glyphHeight); }
  constexpr float scaleHeight() const { return 2.f / surfaceHeight; }
  constexpr float scaleWidth() const { return 2.f / surfaceWidth; }

  friend std::ostream & operator << (std::ostream &, const Dimensions &);

  Dimensions & operator = (const Dimensions &) = default;
};

struct Screen {
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

  Buffer & buffer() { return buffer_; }
  void clear();
  void makeCurrent() const;
  void paint();
  void repaint();
  bool swapGLBuffers() const;
  void write();

private:
  Screen(std::unique_ptr<wayland::Surface> && surface) : surface_(std::move(surface)) { }

  void dimensions();

  freetype::Library freetype_ = {};

  freetype::Face face_ = {};
  std::unique_ptr<wayland::Surface> surface_;

  GLuint glProgram_ = 0;

  Buffer buffer_;
  bool repaint_ = false;
  bool wrap_ = false;

  Dimensions dimensions_;

  struct {
    GLint background = 0;
    GLint color = 0;
    GLint texture = 0;
    GLint vpos = 0;
  } location_;
};
