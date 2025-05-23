#pragma once

#include <iostream>
#include <vector>

#include "buffer.h"
#include "font.h"
#include "freetype.h"
#include "wayland.h"

struct Dimensions {
  uint16_t column = 0;
  uint16_t cursor_left = 10;
  uint16_t cursor_top = 10;
  uint16_t glyphAscender = 0;
  int16_t glyphDescender = 0;
  uint16_t lineHeight = 0;
  uint16_t glyphWidth = 0;
  uint16_t leftPadding = 10;
  uint16_t line = 0;
  int16_t scrollX = 0;
  int16_t scrollY = 0;
  uint16_t surfaceHeight = 0;
  uint16_t surfaceWidth = 0;
  uint16_t bottomPadding = 10;
  constexpr uint16_t columns() const { return std::floor((surfaceWidth - leftPadding * 2) / glyphWidth); }
  constexpr uint16_t lines() const { return std::floor((surfaceHeight - bottomPadding * 2) / lineHeight); }
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

#if 0
  template <class ... Args>
  void loadFace(Args && ... args) {
    face_ = freetype_.load(std::forward<Args>(args)...);
    dimensions();
    repaint_ = true;
  }
#endif

  static Screen New(const wayland::Connection &, Font &&);

  Buffer & buffer() { return buffer_; }
  void clear();

  void changeScrollY(const int32_t);

  void changeScrollX(const int32_t);

  int32_t getColumns() const { return dimensions_.columns(); }
  int32_t getLines() const { return dimensions_.lines(); }

  void decreaseFontSize() { font_.decreaseSize(); dimensions(); }
  void increaseFontSize() { font_.increaseSize(); dimensions(); }

  void makeCurrent() const;
  void paint();
  void repaint();
  void resetScroll() { dimensions_.scrollX = dimensions_.scrollY = 0; }
  bool swapBuffers() const;
  void resize(int32_t, int32_t);
  void write() { repaint_ = true; }

  /* how to handle cursor */
  void setCursor(const uint32_t column, const uint32_t line);

  std::function<void (int32_t, int32_t)> onResize;

private:
  Screen(std::unique_ptr<wayland::Surface> &&, Font &&);

  void dimensions();

  Font font_;

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
