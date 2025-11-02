#include <iostream>

#include "dimensions.h"

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "cursor:"
    << " cursor_column " << d.cursor_column_
    << ", cursor_line " << d.cursor_line_
    << ", columns " << d.columns()
    << ", lines " << d.lines()
    << ", displayed_lines" << d.displayed_lines_
    << ", scrollback_lines " << d.scrollback_lines_
    << "\n"
    << "font:"
    << " glyph_descender " << d.glyph_descender_
    << ", glyph_width " << d.glyph_width_
    << ", line_height " << d.line_height_
    << "\n"
    << "surface:"
    << " surface_height " << d.surface_height_
    << ", surface_width " << d.surface_width_
    << "\n"
    << "scroll_y " << d.scroll_y_;
  return o;
}

void Dimensions::clear() {
  scrollback_lines_ += displayed_lines_ - 1;
  displayed_lines_ = 1;
}

bool Dimensions::new_line() {
  const bool add_scrollback_line = lines() == displayed_lines_;
  if (add_scrollback_line) {
    scrollback_lines_ += 1;
  } else {
    displayed_lines_ = cursor_line_ += 1;
  }
  return add_scrollback_line;
}

void Dimensions::set_cursor(const uint16_t column, const uint16_t line) {
  assert(0 < column);
  assert(columns() > column);
  assert(0 < line);
  assert(lines() > line);
  cursor_column_ = column;
  cursor_line_ = line;
}

Dimensions::operator Rectangle_Y () {
  assert(0 < glyph_width_);
  assert(0 < cursor_line_);
  assert(0 < line_height_);
  assert(0 < cursor_column_);

  return {
    .x = column_to_pixel(cursor_column_),
    .y = line_to_pixel(scrollback_lines_ + cursor_line_),
    .width = glyph_width_,
    .height = line_height_,
  };
}
