#pragma once

#include <iostream>
#include <list>
#include <vector>

#include "buffer.h"
#include "font.h"
#include "freetype.h"
#include "opengl.h"
#include "rune.h"
#include "wayland.h"

struct Dimensions {
  uint16_t column = 0;
  uint16_t cursorLeft = 0;
  uint32_t cursorTop = 0;
  uint16_t glyphAscender = 0;
  int16_t glyphDescender = 0;
  uint16_t lineHeight = 0;
  uint16_t glyphWidth = 0;
  uint16_t leftPadding = 0;
  uint16_t bottomPadding = 0;
  uint16_t line = 0;
  int32_t scrollX = 0;
  uint32_t scrollY = 0;
  uint16_t surfaceHeight = 0;
  uint16_t surfaceWidth = 0;
  constexpr uint16_t columns() const { return std::floor((surfaceWidth - leftPadding * 2) / glyphWidth); }
  constexpr uint16_t lines() const { return std::floor((surfaceHeight - bottomPadding * 2) / lineHeight); }
  constexpr float scaleHeight() const { return 2.f / surfaceHeight; }
  constexpr float scaleWidth() const { return 2.f / surfaceWidth; }

  friend std::ostream & operator << (std::ostream &, const Dimensions &);

  Dimensions & operator = (const Dimensions &) = default;
};

struct Framebuffer {
private:
  struct Entry {
    opengl::Framebuffer framebuffer;
    Rectangle area;
  };

  using Container = std::list<Entry>;

public:
  struct Draw {
    ~Draw();
    auto operator()(Rectangle &&) -> Rectangle;

  private:
    Draw(Framebuffer & framebuffer);
    Framebuffer & framebuffer_;

    friend class Framebuffer;
  };

  Framebuffer(const uint8_t cap = 0) : cap_(cap) { }

  auto draw() -> Draw { return Draw(*this); }; 
  auto paintFrame(const uint16_t frame = 0) -> bool;
  auto repaint(const uint16_t offset = 0, uint32_t scroll = 0) -> void;
  auto resize(const uint16_t, const uint16_t) -> void;
  auto height() const -> uint32_t;

  constexpr auto scaleHeight() -> float const { return 2.f / height_; }
  constexpr auto scaleWidth() -> float const { return 2.f / width_; }

private:
  auto update(Rectangle &) -> void;

  Container container_;
  Container::iterator current_ = container_.end();

  uint16_t height_ = 0;
  uint16_t width_ = 0;
  uint8_t cap_ = 0;
};

struct Screen {
  ~Screen();
  Screen() = delete;

  Screen(const Screen &) = delete;
  Screen(Screen &&);
  Screen & operator = (const Screen &) = delete;
  Screen & operator = (Screen && other) = delete;

  static Screen New(const wayland::Connection &, Font &&);

  void changeScrollY(const int32_t);
  void changeScrollX(const int32_t);
  void resetScroll() { dimensions_.scrollX = dimensions_.scrollY = 0; }

  int32_t getColumns() const { return dimensions_.columns(); }
  int32_t getLines() const { return dimensions_.lines(); }

  void decreaseFontSize() { font_.decreaseSize(); dimensions(); }
  void increaseFontSize() { font_.increaseSize(); dimensions(); }

  void clear();
  void repaint();
  void resize(int32_t, int32_t);
  void pushBack(Rune &&);
  void setCursor(const uint32_t, const uint32_t) { assert(!"UNIMPLEMENTED"); }

  std::function<void (int32_t, int32_t)> onResize;

private:
  Buffer & buffer() { return buffer_; }
  void makeCurrent() const { surface_->egl().makeCurrent(); }
  bool swapBuffers() const { return surface_->egl().swapBuffers(); }
  void draw();

  Screen(std::unique_ptr<wayland::Surface> &&, Font &&);

  void dimensions();

  Font font_;

  std::unique_ptr<wayland::Surface> surface_;

  GLuint glProgram_ = 0;
  Framebuffer framebuffer_ = Framebuffer(/* total number of framebuffers */ 5);
  std::list<Rectangle> rectangles_;

  Buffer buffer_;
  bool repaint_ = false;
  bool repaintFull_ = false;
  bool wrap_ = false;

  Dimensions dimensions_;

  friend class Terminal;
  friend class vt100;
};
