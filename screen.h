#pragma once

#include <iostream>
#include <list>
#include <vector>

#include "buffer.h"
#include "font.h"
#include "freetype.h"
#include "rune.h"
#include "wayland.h"

struct Dimensions {
  uint16_t column = 0;
  int16_t cursorLeft = 0;
  int16_t cursorBottom = 0;
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

struct Stash {
  std::list<Rectangle> rectangles_;
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

  void changeScrollY(const int32_t);
  void changeScrollX(const int32_t);
  void resetScroll() { dimensions_.scrollX = dimensions_.scrollY = 0; }

  int32_t getColumns() const { return dimensions_.columns(); }
  int32_t getLines() const { return dimensions_.lines(); }

  void decreaseFontSize() { font_.decreaseSize(); dimensions(); }
  void increaseFontSize() { font_.increaseSize(); dimensions(); }

  void clear();
  void paint();
  void repaint();
  void resize(int32_t, int32_t);
  void pushBack(Rune &&);
  void setCursor(const uint32_t, const uint32_t) { assert(!"UNIMPLEMENTED"); }

  std::function<void (int32_t, int32_t)> onResize;

private:
  Buffer & buffer() { return buffer_; }
  void makeCurrent() const { surface_->egl().makeCurrent(); }
  bool swapBuffers() const { return surface_->egl().swapBuffers(); }

  Screen(std::unique_ptr<wayland::Surface> &&, Font &&);

  void dimensions();

  Font font_;

  std::unique_ptr<wayland::Surface> surface_;

  GLuint glProgram_ = 0;

  Buffer buffer_;
  bool repaint_ = false;
  bool repaintFull_ = false;
  bool wrap_ = false;

  Dimensions dimensions_;

  std::list<Rectangle> rectangles_;

  friend class Terminal;
  friend class vt100;
};
