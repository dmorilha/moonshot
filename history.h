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
  auto at(const uint16_t, const uint16_t) const -> const rune::Rune &;
  auto columns() const { return columns_; }
  auto erase_display() -> void;
  auto erase_scrollback() -> void { scrollback_.clear(); }
  auto carriage_return() -> void;
  auto count_lines(ReverseIterator &, const ReverseIterator &, const uint64_t limit = 0) const -> uint64_t;
  auto emplace(rune::Rune) -> void;
  auto erase(const int) -> void;
  auto erase_line_right() -> void;
  auto insert(const int) -> void;
  auto lines() const -> std::size_t { return scrollback_lines_; }
  auto new_line() -> void;
  auto print() const -> void;
  auto rbegin() const -> ReverseIterator { return scrollback_.rbegin(); }
  auto rend() const -> ReverseIterator { return scrollback_.rend(); }
  auto resize(const uint16_t, const uint16_t) -> void;
  auto reverse_iterator(const uint64_t) -> ReverseIterator;
  auto scrollback_size() const -> uint64_t { return scrollback_.size(); }
  auto size() const -> uint64_t;

  // cursor
  auto move_cursor(const int, const int) -> void;
  auto move_cursor_backward(const int) -> void;
  auto move_cursor_down(const int) -> void;
  auto move_cursor_forward(const int) -> void;
  auto move_cursor_up(const int) -> void;

private:
  auto check_active_size() const -> uint32_t;

  auto scrollback() -> void;

  Container active_; // circular buffer representing the screen
  Container scrollback_; // scrollback buffer

  uint16_t columns_ = 0;
  uint32_t active_size_ = 0;
  uint32_t first_ = 0;
  uint32_t last_ = 0;
  uint64_t scrollback_lines_ = 0;
};
