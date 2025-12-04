#pragma once

#include <list>
#include <set>

#include "character-map.h"
#include "dimensions.h"
#include "font.h"
#include "freetype.h"
#include "history.h"
#include "opengl.h"
#include "rune.h"
#include "types.h"
#include "wayland.h"

struct Damage {
  using Container = std::set<Rectangle>;
  auto area() const -> uint32_t;
  auto clear() -> void { container_.clear(); }
  auto emplace(Rectangle &&) -> void;
  auto empty() const -> bool { return container_.empty(); }

  template <typename Container>
  void transfer(Container & c) {
    while ( ! container_.empty()) {
      c.emplace(c.cend(), container_.extract(container_.begin()).value());
    }
  }

  Container container_;
};

struct Pages {
private:
  struct Entry {
    opengl::Framebuffer framebuffer;
    opengl::Framebuffer alternative;
    Rectangle_Y area;
    uint64_t index = 0;
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
  auto repaint(const Rectangle, const int64_t, const bool alternative = false) -> void;
  auto reset(const uint16_t, const uint16_t) -> void;

  auto has_alternative() const -> bool;
  auto front_index() const -> uint64_t { return container_.empty() ? 0 : container_.front().index; }
  auto front_y() const -> int64_t { return container_.empty() ? 0 : container_.front().area.y; }
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

  // cursor
  auto move_cursor_backward(const int) -> void;
  auto move_cursor_down(const int) -> void;
  auto move_cursor_forward(const int) -> void;
  auto move_cursor_up(const int) -> void;
  auto move_cursor(const int, const int) -> void;

  auto reverse_line_feed() -> void;

  auto backspace() -> void;
  auto begin() -> void { }
  auto changeScrollY(int32_t) -> void;
  auto erase_display() -> void;
  auto erase_scrollback() -> void;
  auto commit() -> void { }
  auto drag(const uint16_t, const uint16_t) -> void;
  auto erase(const int) -> void;
  auto erase_line_right() -> void;
  auto column() const -> int32_t { return dimensions_.cursor_column(); }
  auto columns() const -> int32_t { return dimensions_.columns(); }
  auto line() const -> int32_t { return dimensions_.cursor_line(); }
  auto lines() const -> int32_t { return dimensions_.lines(); }
  auto insert(const int) -> void;
  auto pushBack(rune::Rune &&) -> void;
  auto repaint(const bool force = false, const bool alternative = false) -> void;
  auto resetScroll() -> void { dimensions_.scroll_y(0); }
  auto resize(const uint16_t, const uint16_t) -> void;
  auto setTitle(const std::string &) -> void;
  auto shouldRepaint() -> bool { return FULL == repaint_; }
  auto startSelection(const uint16_t, const uint16_t) -> void;

  std::function<void (int32_t, int32_t)> onResize;

private:
  Screen(std::unique_ptr<wayland::Surface> &&);

  auto cursor(const uint64_t) const -> void;
  auto draw() -> void;
  auto history() -> History & { return history_; }
  auto makeCurrent() const -> void { surface_->egl().makeCurrent(); }
  auto overflow() -> int32_t;
  auto pushCharacter(rune::Rune) -> uint16_t;
  auto recreateFromActiveHistory() -> void;
  auto recreateFromScrollback(const uint64_t index) -> void;
  auto renderCharacter(const Rectangle &, const rune::Rune &) -> void;
  auto select(const Rectangle & rectangle) -> void;
  auto swapBuffers(bool fullSwap = true) -> void;

  CharacterMap characters_;
  Dimensions dimensions_;
  History history_;
  Pages pages_{/* total number of entries */ 3};
  Damage damage_;
  Repaint repaint_ = NO;
  opengl::Shader glProgram_;
  std::unique_ptr<wayland::Surface> surface_;
};
