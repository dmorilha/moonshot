#pragma once

#include <iostream>
#include <vector>

#include "buffer.h"
#include "freetype.h"
#include "wayland.h"

struct Dimensions {
  uint16_t column = 0;
  uint16_t cursor_left = 10;
  uint16_t cursor_top = 10;
  uint16_t glyphAscender = 0;
  int16_t glyphDescender = 0;
  uint16_t glyphHeight = 0;
  uint16_t glyphWidth = 0;
  uint16_t leftPadding = 10;
  uint16_t line = 0;
  int16_t scrollX = 0;
  int16_t scrollY = 0;
  uint16_t surfaceHeight = 0;
  uint16_t surfaceWidth = 0;
  uint16_t bottomPadding = 10;
  constexpr uint16_t columns() const { return std::floor((surfaceWidth - leftPadding * 2) / glyphWidth); }
  constexpr uint16_t lines() const { return std::floor((surfaceHeight - bottomPadding * 2) / glyphHeight); }
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
    repaint_ = true;
  }

  static Screen New(const wayland::Connection &);

  Buffer & buffer() { return buffer_; }
  void clear();

  // scrolling like this seems inefficient
  void changeScrollY(int32_t value) {
    dimensions_.scrollY += value * 2;
    repaint_ = true;
  }

  void changeScrollX(int32_t value) {
    dimensions_.scrollX += value * 2;
    repaint_ = true;
  }

  int32_t getColumns() const { return dimensions_.columns(); }
  int32_t getLines() const { return dimensions_.lines(); }
  void makeCurrent() const;
  void paint();
  void repaint();
  bool swapGLBuffers() const;
  void resize(int32_t, int32_t);
  void write() { repaint_ = true; }

  std::function<void (int32_t, int32_t)> onResize;

private:
  Screen(std::unique_ptr<wayland::Surface> &&);

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
