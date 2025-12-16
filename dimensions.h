// Copyright Daniel Morilha 2025

#pragma once

#include <cmath>

#include <cassert>

#include "freetype.h"
#include "types.h"

class Dimensions {
public:
  Dimensions() = default;
  Dimensions(const Dimensions &) = default;
  Dimensions(Dimensions &&) = delete;
  Dimensions & operator = (const Dimensions &) = default;
  Dimensions & operator = (Dimensions &&) = delete;

  auto area() const -> uint32_t { return surface_width_ * surface_height_; }
  auto backspace() -> void;
  auto cursor_column() const { return cursor_column_; }
  auto cursor_column(const uint16_t) -> void;
  auto cursor_line() const { return cursor_line_; }
  auto cursor_line(const uint16_t) -> void;
  auto displayed_lines() const { return displayed_lines_; }
  auto displayed_lines(const uint16_t v) -> void;
  auto enable_overflow(const bool v) { enable_overflow_ = v; }
  auto erase_display() -> void;
  auto glyph_descender() const { return glyph_descender_; }
  auto glyph_descender(const auto v) { glyph_descender_ = v; }
  auto glyph_width() const { return glyph_width_; }
  auto glyph_width(const auto v) { glyph_width_ = v; }
  auto line_height() const { return line_height_; }
  auto line_height(const auto v) { line_height_ = v; }
  auto move_cursor(const uint16_t, const uint16_t) -> void;
  auto new_line() -> bool;
  auto overflow() const { return enable_overflow_ && overflow_; }
  auto reset(freetype::Face & face, const uint16_t, const uint16_t) -> void;
  auto scroll_y() const { return scroll_y_; }
  auto scroll_y(const auto v) { scroll_y_ = v; }
  auto scrollback_lines() const { return scrollback_lines_; }
  auto surface_height() const { return surface_height_; }
  auto surface_width() const { return surface_width_; }
  auto wrap_next() const { return wrap_next_; }

  explicit operator Rectangle () const;
  explicit operator Rectangle_Y () const;

  constexpr int32_t column_to_pixel(const uint16_t column) const {
    assert(0 < glyph_width_);
    assert(0 < column);
    return (column - 1) * glyph_width_;
  }

  constexpr uint16_t columns() const {
    assert(0 < surface_width_);
    assert(0 < glyph_width_);
    assert(glyph_width_ < surface_width_);
    return std::ceil(surface_width_ / glyph_width_);
  }

  constexpr uint16_t lines() const {
    assert(0 < surface_height_);
    assert(0 < line_height_);
    assert(line_height_ < surface_height_);
    return std::ceil(surface_height_ / line_height_);
  }

  constexpr uint64_t line_to_pixel(const uint64_t line) const {
    assert(0 < line_height_);
    assert(0 < line);
    return (line - 1) * line_height_;
  }

  constexpr uint16_t page_size() const {
    assert(0 < line_height_);
    return line_height_ * lines();
  }

  constexpr uint8_t remainder() const {
    assert(0 < surface_height_);
    assert(0 < line_height_);
    assert(surface_height_ >= line_height_);
    return surface_height_ % line_height_;
  }

  constexpr uint64_t pixel_to_line(const uint64_t y) const {
    assert(0 < surface_height_);
    assert(0 < line_height_);
    assert(line_height_ <= surface_height_);
    return y / line_height_;
  }

  constexpr float scale_height() const {
    assert(0 < surface_height_);
    return 2.f / surface_height_;
  }

  constexpr float scale_width() const {
    assert(0 < surface_width_);
    return 2.f / surface_width_;
  }

private:
  // current font metrics
  int16_t glyph_descender_ = 0;
  uint16_t glyph_width_ = 0;
  uint16_t line_height_ = 0;

  // the next character will be displayed at
  uint16_t cursor_column_ = 1; // 65k, it goes 1 up to a ... ~thousand
  uint16_t cursor_line_ = 1; // 65k, it goes 1 up to a ... ~thousand

  uint16_t surface_height_ = 0; // pixels
  uint16_t surface_width_ = 0; // pixels

  uint64_t scroll_y_ = 0; // pixels

  uint16_t displayed_lines_ = 1; // 65k it goes 1 up to a ... ~thousand
  uint64_t scrollback_lines_ = 0; // memory is the limit really.
  bool overflow_ = false; // whether the cursor reached the end of the screen.
  bool wrap_next_ = false; // cursor went beyond columns.
  bool enable_overflow_ = true; // enable / disable overflow.

  friend std::ostream & operator << (std::ostream &, const Dimensions &);
};
