#include "buffer.h"

Buffer::Buffer() {
  lines_.emplace_back(Line{});
}

void Buffer::clear() {
  for (Line & line : lines_) {
    line.clear();
  }
  lines_.clear();
}

void Buffer::push_back(const Rune & c) {
  bool newLine = false;
  switch (c.character) {
    case '\a': // BELL
      // what should we do?
      return;
      break;

    case '\b': // BACKSPACE
      {
        if ( ! lines_.back().empty()) {
          lines_.back().erase(lines_.back().end());
        }
        return;
      }
      break;

    case '\n': // NEW LINE
      newLine = true;
      break;

    default:
      break;
  }

  lines_.back().push_back(c);

  if (newLine) {
    lines_.emplace_back(Line{});
  }
}
