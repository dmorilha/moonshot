#pragma once

#include <list>

#include "buffer.h"
#include "character-map.h"
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
    Rectangle_Y area;
  };

  using Container = std::list<Entry>;

public:
  struct Draw {
    ~Draw();
    auto operator () (Rectangle_Y, const Color & color = {0.f, 0.f, 0.f, 0.f}) -> Rectangle;
  private:
    Draw(Framebuffer & framebuffer);
    Framebuffer & framebuffer_;
    friend class Framebuffer;
  };

  Framebuffer(const uint8_t cap = 0) : cap_(cap) { }

  auto draw() -> Draw { return Draw(*this); }; 
  auto paintFrame(const uint16_t frame = 0) -> bool;
  auto repaint(const Rectangle, const uint64_t) -> void;
  auto resize(const uint16_t, const uint16_t) -> void;
  auto height() const -> uint32_t;

  constexpr auto scale_height() -> float const { return 2.f / height_; }
  constexpr auto scale_width() -> float const { return 2.f / width_; }

private:
  auto update(Rectangle_Y &) -> void;

  Container container_;
  Container::iterator current_ = container_.end();

  opengl::Shader glProgram_;

  uint16_t height_ = 0;
  uint16_t width_ = 0;
  uint8_t cap_ = 0;

  friend class Screen;
};

struct Screen {
  ~Screen();
  Screen() = delete;

  Screen(const Screen &) = delete;
  Screen(Screen &&);
  Screen & operator = (const Screen &) = delete;
  Screen & operator = (Screen && other) = delete;

  static Screen New(const wayland::Connection &);

  void changeScrollY(int32_t);
  void changeScrollX(const int32_t);
  void resetScroll() { dimensions_.scroll_y = 0; }

  int32_t getColumns() const { return dimensions_.columns(); }
  int32_t getLines() const { return dimensions_.lines(); }
  int32_t getColumn() const { return dimensions_.cursor_column; }
  int32_t getLine() const { return dimensions_.cursor_line; }

  #if 0
  void decreaseFontSize() { font_.decreaseSize(); dimensions(); }
  void increaseFontSize() { font_.increaseSize(); dimensions(); }
  #endif

  void pushBack(rune::Rune &&);

  void clear();
  void backspace();
  void repaint();
  void resize(uint16_t, uint16_t);
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

  Rectangle printCharacter(Framebuffer::Draw & drawer, const Rectangle_Y &, rune::Rune);

  Screen(std::unique_ptr<wayland::Surface> &&);

  void dimensions();

  std::unique_ptr<wayland::Surface> surface_;

  opengl::Shader glProgram_;
  Framebuffer framebuffer_ = Framebuffer(/* total number of entries */ 10);
  std::list<Rectangle> rectangles_;

  Buffer buffer_;

  enum {
    NO,
    DRAW,
    SCROLL,
  } repaint_ = NO;

  bool repaintFull_ = false;

  Dimensions dimensions_;

  CharacterMap characters_;
};
