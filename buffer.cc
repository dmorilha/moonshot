#include <iostream>

#include "buffer.h"

Buffer::Buffer() {
  rows_.emplace_back(Row{});
}

void Buffer::clear() {
  for (Row & row : rows_) {
    row.clear();
  }
  rows_.clear();
}

void Buffer::push_back(const Char & c) {
  bool newLine = false;
  switch (c.character) {
    case '\a': // BELL
      // what should we do?
      return;
      break;

    case '\b': // BACKSPACE
      {
        if ( ! rows_.back().empty()) {
          rows_.back().erase(rows_.back().end());
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

  #if 0
  assert(0 < columns_);
  if (columns_ <= rows_.back().size()) {
    newLine = true;
  }
  #endif

  rows_.back().push_back(c);

  if (newLine) {
    rows_.emplace_back(Row{});
  }
}
