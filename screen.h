#pragma once

#include <iostream>
#include <list>
#include <vector>

#include "buffer.h"
#include "dimensions.h"
#include "font.h"
#include "freetype.h"
#include "opengl.h"
#include "rune.h"
#include "types.h"
#include "wayland.h"

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
    auto operator()(Rectangle, const Color & color = {0.f, 0.f, 0.f, 0.f}) -> Rectangle;

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
  void resetScroll() { dimensions_.scrollY = 0; }

  int32_t getColumns() const { return dimensions_.columns(); }
  int32_t getLines() const { return dimensions_.lines(); }
  int32_t getColumn() const { return dimensions_.column; }
  int32_t getLine() const { return dimensions_.line; }

  void decreaseFontSize() { font_.decreaseSize(); dimensions(); }
  void increaseFontSize() { font_.increaseSize(); dimensions(); }

  void pushBack(rune::Rune &&);

  void clear();
  void backspace();
  void repaint();
  void resize(int32_t, int32_t);
  void setCursor(uint16_t, uint16_t);
  void setPosition(uint16_t, uint16_t);
  void setTitle(const std::string);

  void drag(const uint16_t, const uint16_t);

  void startSelection(uint16_t, uint16_t);
  void endSelection();

  std::function<void (int32_t, int32_t)> onResize;

private:
  Buffer & buffer() { return buffer_; }
  void makeCurrent() const { surface_->egl().makeCurrent(); }
  void swapBuffers() ;

  void draw();

  void select(const Rectangle & rectangle);

  Rectangle printCharacter(Framebuffer::Draw & drawer, const Rectangle &, rune::Rune);

  Screen(std::unique_ptr<wayland::Surface> &&, Font &&);

  void dimensions();

  Font font_;

  std::unique_ptr<wayland::Surface> surface_;

  GLuint glProgram_ = 0;
  Framebuffer framebuffer_ = Framebuffer(/* total number of framebuffers */ 5);
  std::list<Rectangle> rectangles_;

  Buffer buffer_;

  enum {
    NO,
    DRAW,
    SCROLL,
  } repaint_ = NO;

  bool repaintFull_ = false;

  Dimensions dimensions_;
};
