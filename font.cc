#include <stdexcept>

#include "font.h"

Font Font::New(Paths && paths) {
  if (paths.regular.empty()) {
    throw std::runtime_error("error");
  }
  return Font(std::move(paths));
}

freetype::Face & Font::bold() {
  if ( ! static_cast<bool>(bold_)) {
    if (paths_.bold.empty()) {
      return regular();
    }
    bold_ = freetype_.load(paths_.bold, paths_.size);
  }
  return bold_;
}

freetype::Face & Font::boldItalic() {
  if ( ! static_cast<bool>(boldItalic_)) {
    if (paths_.boldItalic.empty()) {
      return regular();
    }
    boldItalic_ = freetype_.load(paths_.boldItalic, paths_.size);
  }
  return boldItalic_;
}

freetype::Face & Font::italic() {
  if ( ! static_cast<bool>(italic_)) {
    if (paths_.italic.empty()) {
      return regular();
    }
    italic_ = freetype_.load(paths_.italic, paths_.size);
  }
  return italic_;
}

freetype::Face & Font::regular() {
  if ( ! static_cast<bool>(regular_)) {
    regular_ = freetype_.load(paths_.regular, paths_.size);
  }
  return regular_;
}

void Font::clear() {
  boldItalic_ = std::move(freetype::Face());
  bold_ = std::move(freetype::Face());
  italic_ = std::move(freetype::Face());
  regular_ = std::move(freetype::Face());
}
  
