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
  overflow_ = false;
}

bool Dimensions::new_line() {
  const bool add_scrollback_line = lines() <= displayed_lines_;
  if (add_scrollback_line) {
    overflow_ = 0 < remainder();
    scrollback_lines_ += 1;
    displayed_lines_ = cursor_line_ = lines();
  } else {
    assert( ! overflow_);
    displayed_lines_ = cursor_line_ += 1;
  }
  return add_scrollback_line;
}

void Dimensions::displayed_lines(const uint16_t v) {
  assert(lines() >= displayed_lines_);
  displayed_lines_ = v;
}

void Dimensions::set_cursor(const uint16_t column, const uint16_t line) {
  assert(0 < column);
  assert(columns() >= column);
  assert(0 < line);
  assert(lines() >= line);
  cursor_column_ = column;
  cursor_line_ = line;
}

Dimensions::operator Rectangle_Y () const {
  assert(0 < glyph_width_);
  assert(0 < cursor_line_);
  assert(0 < line_height_);
  assert(0 < cursor_column_);

  return {
    .x = column_to_pixel(cursor_column_),
    .y = static_cast<int64_t>(line_to_pixel(scrollback_lines_ + cursor_line_)),
    .width = glyph_width_,
    .height = line_height_,
  };
}

Dimensions::operator Rectangle () const {
  assert(0 < surface_height_);
  assert(0 < glyph_width_);
  assert(0 < cursor_line_);
  assert(0 < line_height_);
  assert(0 < cursor_column_);

  int32_t y = surface_height_ - static_cast<int32_t>(line_to_pixel(cursor_line_ + 1));

  if (overflow_) {
    y -= remainder();
  }

  return {
    .x = column_to_pixel(cursor_column_),
    .y = y,
    .width = glyph_width_,
    .height = line_height_,
  };
}

void Dimensions::cursor_column(const uint16_t v) {
  assert(0 < v);
  wrap_next_ = columns() < v;
  cursor_column_ = v;
}

void Dimensions::reset(freetype::Face & face, const uint16_t width, const uint16_t height) {
  assert(0 < width);
  assert(0 < height);
  surface_width_ = width;
  surface_height_ = height;

  glyph_descender_ = face.descender();

  assert(0 < face.lineHeight());
  line_height_ = face.lineHeight();

  assert(0 < face.glyphWidth());
  glyph_width_ = face.glyphWidth();

  cursor_column_ = cursor_line_ = displayed_lines_ = 1;
  overflow_ = wrap_next_ = false;
  scroll_y_ = scrollback_lines_ = 0;
}
