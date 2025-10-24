#pragma once

#include <cmath>

#include <cassert>

#include "types.h"

struct Dimensions {
  // current font metrics
  int16_t glyph_descender = 0;
  uint16_t glyph_width = 0;
  uint16_t line_height = 0;

  // the next character will be displayed at
  uint16_t cursor_column = 1; // 65k, it goes 1 up to a ... ~thousand
  uint16_t cursor_line = 1; // 65k, it goes 1 up to a ... ~thousand

  uint16_t surface_height = 0; // is it pixels?
  uint16_t surface_width = 0; // is it pixels?

  uint64_t scroll_y = 0; // is it pixels?

  uint16_t displayed_lines = 1; //65k it goes 1 up to a ... ~thousand
  uint64_t scrollback_lines = 0; //memory is the limit really.

  Dimensions() = default;
  Dimensions(const Dimensions &) = default;
  Dimensions(Dimensions &&) = delete;
  Dimensions & operator = (const Dimensions &) = default;
  Dimensions & operator = (Dimensions &&) = delete;

  constexpr void new_line() {
    if (lines() > displayed_lines) {
      displayed_lines = cursor_line += 1;
    } else {
      scrollback_lines += 1;
    }
  }

  constexpr float scale_height() const {
    assert(0 < surface_height);
    return 2.f / surface_height;
  }

  constexpr float scale_width() const {
    assert(0 < surface_width);
    return 2.f / surface_width;
  }

  constexpr uint16_t columns() const {
    assert(0 < surface_width);
    assert(0 < glyph_width);
    assert(glyph_width < surface_width);
    return std::ceil(surface_width / glyph_width);
  }

  constexpr uint16_t lines() const {
    assert(0 < surface_height);
    assert(0 < line_height);
    assert(line_height < surface_height);
    return std::ceil(surface_height / line_height);
  }

  constexpr uint64_t line_to_pixel(const uint64_t line) const {
    assert(0 < line_height);
    assert(0 < line);
    return (line - 1) * line_height;
  }

  constexpr int32_t column_to_pixel(const uint16_t column) const {
    assert(0 < glyph_width);
    assert(0 < column);
    return (column - 1) * glyph_width;
  }
  
  constexpr void set_cursor(const uint16_t column, const uint16_t line) {
    assert(0 < column);
    assert(columns() > column);
    assert(0 < line);
    assert(lines() > line);
    cursor_column = column;
    cursor_line = line;
  }

  constexpr operator Rectangle_Y () {
    assert(0 < glyph_width);
    assert(0 < cursor_line);
    assert(0 < line_height);
    assert(0 < cursor_column);

    return {
      .x = column_to_pixel(cursor_column),
      .y = line_to_pixel(scrollback_lines + cursor_line),
      .width = glyph_width,
      .height = line_height,
    };
  }

  friend std::ostream & operator << (std::ostream &, const Dimensions &);
};
