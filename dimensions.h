#pragma once

#include <cmath>
#include <iostream>

#include "types.h"

/*
 * move offset within
 * pointer line/column can be relative to view port or absolute
 */

struct Dimensions {
  uint16_t offset = 0;
  uint16_t column = 1;
  uint16_t cursorLeft = 0;
  uint32_t cursorTop = 0;
  uint16_t glyphAscender = 0;
  int16_t glyphDescender = 0;
  uint16_t lineHeight = 0;
  uint16_t glyphWidth = 0;
  uint16_t leftPadding = 0;
  uint16_t bottomPadding = 0;
  uint16_t line = 1;
  uint16_t pointerLine = 0;
  uint16_t pointerColumn = 0;
  uint32_t screenTop = 0;
  int32_t scrollX = 0;
  uint32_t scrollY = 0;
  uint16_t surfaceHeight = 0;
  uint16_t surfaceWidth = 0;
  uint64_t totalLines = 1;

  Rectangle selection = {0, 0, 0, 0};

  constexpr uint16_t columns() const {
    return std::floor((surfaceWidth - leftPadding * 2) / glyphWidth);
  }

  constexpr uint16_t lines() const {
    return std::floor((surfaceHeight - bottomPadding * 2) / lineHeight);
  }

  constexpr float scaleHeight() const {
    return 2.f / surfaceHeight;
  }

  constexpr float scaleWidth() const {
    return 2.f / surfaceWidth;
  }

  constexpr uint64_t bufferedLines() const {
    return std::max<int64_t>(0, totalLines - lines());
  }

  constexpr uint16_t pixelToLine(const uint16_t y) const {
    return (screenTop - scrollY + y) / lineHeight + 1;
  }

  constexpr int32_t lineToPixel(const uint16_t line) const {
    assert(0 < line);
    return (line - 1) * lineHeight;
  }

  constexpr int32_t columnToPixel(const uint16_t column) const {
    assert(0 < column);
    return (column - 1) * glyphWidth;
  }

  friend std::ostream & operator << (std::ostream &, const Dimensions &);

  Dimensions & operator = (const Dimensions &) = default;
};
