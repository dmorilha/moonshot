#pragma once

#include <list>

#include "character-map.h"
#include "dimensions.h"
#include "font.h"
#include "freetype.h"
#include "history.h"
#include "opengl.h"
#include "rune.h"
#include "types.h"
#include "wayland.h"

struct Pages {
private:
  struct Entry {
    opengl::Framebuffer framebuffer;
    opengl::Framebuffer alternative;
    Rectangle_Y area;
    uint64_t buffer_index = 0;
  };

  using Container = std::list<Entry>;

public:
  struct Drawer {
    ~Drawer();
    auto alternative() const -> bool; 
    auto clear(const Color & color) const -> void;
    auto create_alternative() const -> void;
    const Rectangle target;
  private:
    Drawer(const Pages &, Entry &, Rectangle &&);
    Entry & entry_;
    const Pages & pages_;
    friend class Pages;
  };

  Pages(const uint8_t cap = 0) : cap_(cap) { }

  auto draw(Rectangle_Y, const uint64_t) -> Drawer;
  auto emplace_front(const int32_t) -> Entry &;
  auto is_current(Entry & entry) const -> bool { return current_->framebuffer == entry.framebuffer; }
  auto paint(const uint16_t frame = 0) -> bool;
  auto repaint(const Rectangle, const uint64_t, const bool alternative = false) -> void;
  auto reset(const uint16_t, const uint16_t) -> void;

  auto has_alternative() const -> bool;
  auto first_buffer_index() const -> uint64_t { return container_.empty() ? 0 : container_.front().buffer_index; }
  auto first_y() const -> uint64_t { return container_.empty() ? 0 : container_.front().area.y; }
  auto total_height() const -> uint32_t;

  constexpr auto height() const { return height_; }
  constexpr auto scale_height() const { return 2.f / height_; }
  constexpr auto scale_width() const { return 2.f / width_; }

private:
  auto entry(const Rectangle_Y &, const uint64_t) -> Entry;
  auto update(Rectangle_Y &, const uint64_t) -> Entry &;

  Container container_;
  Container::iterator current_ = container_.end();

  opengl::Shader glProgram_;

  uint16_t height_ = 0;
  uint16_t width_ = 0;
  uint8_t cap_ = 0;

  friend class Screen;
};

struct Screen {
  enum Repaint {
    NO,
    PARTIAL,
    FULL,
  };

  static Screen New(const wayland::Connection &);

  ~Screen() = default;
  Screen() = delete;

  Screen(const Screen &) = delete;
  Screen(Screen &&);
  Screen & operator = (const Screen &) = delete;
  Screen & operator = (Screen && other) = delete;

  auto backspace() -> void;
  auto changeScrollY(int32_t) -> void;
  auto clear() -> void;
  auto clearScrollback() -> void;
  auto drag(const uint16_t, const uint16_t) -> void;
  auto EL() -> void;
  auto endSelection() -> void;
  auto getColumn() const -> int32_t { return dimensions_.cursor_column(); }
  auto getColumns() const -> int32_t { return dimensions_.columns(); }
  auto getLine() const -> int32_t { return dimensions_.cursor_line(); }
  auto getLines() const -> int32_t { return dimensions_.lines(); }
  auto pushBack(rune::Rune &&) -> void;
  auto recreateFromBuffer(const uint64_t index) -> void;
  auto repaint(const bool force = false, const bool alternative = false) -> void;
  auto resetScroll() -> void { dimensions_.scroll_y(0); }
  auto resize(const uint16_t, const uint16_t) -> void;
  auto setCursor(const uint16_t, const uint16_t) -> void;
  auto setPosition(const uint16_t, const uint16_t) -> void;
  auto setTitle(const std::string &) -> void;
  auto startSelection(const uint16_t, const uint16_t) -> void;

  std::function<void (int32_t, int32_t)> onResize;

private:
  Screen(std::unique_ptr<wayland::Surface> &&);

  auto history() -> History & { return history_; }
  auto countLines(History::ReverseIterator &, const History::ReverseIterator &, const uint64_t limit = 0) const -> uint64_t;
  auto draw() -> void;
  auto makeCurrent() const -> void { surface_->egl().makeCurrent(); }
  auto overflow() -> int32_t;
  auto renderCharacter(const Rectangle &, const rune::Rune &) -> void;
  auto select(const Rectangle & rectangle) -> void;
  auto swapBuffers(const bool full = true) -> void;

  std::unique_ptr<wayland::Surface> surface_;
  opengl::Shader glProgram_;
  Pages pages_{/* total number of entries */ 3};
  std::list<Rectangle> rectangles_;
  History history_;
  Repaint repaint_ = NO;
  Dimensions dimensions_;
  CharacterMap characters_;
};
