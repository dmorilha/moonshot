#pragma once

#include <deque>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

#include <cassert>
#include <cstdint>

#include "rune.h"

class History {
  using Container = std::vector<rune::Rune>;

public:
  using Iterator = Container::const_iterator;
  using ReverseIterator = Container::const_reverse_iterator;

  auto active_size() const -> uint32_t { return active_size_; }
  auto at(const uint32_t) const -> const rune::Rune &;
  auto clear() -> void;
  auto countLines(ReverseIterator &, const ReverseIterator &, const uint64_t limit = 0) const -> uint64_t;
  auto emplace(rune::Rune &&) -> void;
  auto erase(const int) -> void;
  auto erase_line_right() -> void;
  auto insert(const int) -> void;
  auto lines() const -> std::size_t { return scrollback_lines_; }
  auto next(const uint32_t) const -> const rune::Rune &;
  auto print() const -> void;
  auto rbegin() const -> ReverseIterator { return scrollback_.rbegin(); }
  auto rend() const -> ReverseIterator { return scrollback_.rend(); }
  auto resize(const uint16_t, const uint16_t) -> void;
  auto reverseIterator(const uint64_t) -> ReverseIterator;
  auto scrollback_size() const -> uint64_t { return scrollback_.size(); }
  auto setCursor(const uint16_t, const uint16_t) -> void;
  auto size() const -> uint64_t { return scrollback_size() + active_size_; }

  // cursor
  auto move_cursor_backward(const int) -> void;
  auto move_cursor_down(const int) -> void;
  auto move_cursor_forward(const int) -> void;
  auto move_cursor_up(const int) -> void;

private:
  auto column() const -> uint16_t {
    assert(0 < columns_);
    assert(0 == start_ % columns_);
    return index_ % columns_;
  }

  auto check_active_size() const -> uint32_t;

  auto line() const -> uint16_t;

  Container active_; // circular buffer representing the screen
  Container scrollback_; // scrollback buffer
  uint32_t active_size_ = 0;
  uint32_t index_ = 0; // current index into the displayed lines buffer.
  uint32_t returned_ = 0; // how many characters it has returned.
  uint32_t start_ = 0; // a multiple of columns representing column 1, line 1.
  uint64_t columns_ = 0;
  uint64_t scrollback_lines_ = 0;

  bool carriage_return_ = false;
};
