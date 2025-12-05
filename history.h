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
  auto alternative(const bool) -> void;
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
  auto print_scrollback() const -> void;
  auto rbegin() const -> ReverseIterator { return scrollback_.rbegin(); }
  auto rend() const -> ReverseIterator { return scrollback_.rend(); }
  auto resize(const uint16_t, const uint16_t) -> void;
  auto reverse_iterator(const uint64_t) -> ReverseIterator;
  auto scrollback_size() const -> uint64_t { return scrollback_.size(); }
  auto size() const -> uint64_t;

  // cursor, manipulates the last_ position.
  auto get_cursor() const -> std::pair<uint16_t, uint16_t>;
  auto move_cursor(const int, const int) -> void;
  auto move_cursor_backward(const int) -> void;
  auto move_cursor_down(const int) -> void;
  auto move_cursor_forward(const int) -> void;
  auto move_cursor_up(const int) -> void;

private:
  /*
   * The difference between `scrollback_` and `active_` is that `scrollback_`
   * is a compressed representation of the screen, while `active_` is expanded
   * in the sense that each character is in the right column and line
   * coordinate, considering a circular buffer.
   * Nonprintables are mostly excluded, and the new line character goes at
   * "column 0" in the `active_` container, therefore there is one additional
   * column.
   * It should be fairly simple to transform from `scrollback_` back to
   * `active_`.
   */

  auto check_size(const Container &) const -> uint32_t;

  auto scrollback() -> void;

  Container scrollback_; // scrollback buffer
  uint64_t scrollback_lines_ = 0;

  Container active_; // circular buffer representing the screen
  uint16_t columns_ = 0;
  uint32_t active_size_ = 0;
  uint32_t first_ = 0;
  uint32_t last_ = 0;

  Container saved_;
  uint16_t saved_columns_ = 0;
  uint32_t saved_first_ = 0;
  uint32_t saved_last_ = 0;
  uint32_t saved_size_ = 0;
};
