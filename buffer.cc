#include <iostream>

#include "buffer.h"

void Buffer::clear() {
  container_.clear();
  lines_.clear();
  lines_.push_back(0);
  state_ = INITIAL;
}

void Buffer::push_back(Rune && rune) {
  bool newLine = false;
  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\a': // BELL
      // what should we do?
      return;
      break;

    case L'\b': // BACKSPACE
      if (L'\n' != container_.back() && ! container_.empty()) {
        container_.erase(container_.end());
        --lines_.back();
      }
      return;
      break;

    case L'\n': // NEW LINE
      newLine = true;
      break;

    default:
      break;
    }
  }

  container_.emplace_back(rune);

  ++lines_.back();
  if (newLine) {
    lines_.push_back(0);
    state_ = INITIAL;
  }
}
