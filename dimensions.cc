#include <iostream>

#include "dimensions.h"

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "cursor:"
    << " cursor_column " << d.cursor_column
    << ", cursor_line " << d.cursor_line
    << ", columns " << d.columns()
    << ", lines " << d.lines()
    << ", displayed_lines" << d.displayed_lines
    << ", scrollback_lines " << d.scrollback_lines
    << "\n"
    << "font:"
    << " glyph_descender " << d.glyph_descender
    << ", glyph_width " << d.glyph_width
    << ", line_height " << d.line_height
    << "\n"
    << "surface:"
    << " surface_height " << d.surface_height
    << ", surface_width " << d.surface_width 
    << "\n"
    << "scroll_y " << d.scroll_y;
  return o;
}
