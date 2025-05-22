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

const Buffer::Line Buffer::const_reverse_iterator::operator * () const {
  const std::size_t size = buffer_.container_.size();
  switch (line_) {
  case 0:
    throw std::runtime_error("cannot dereference end iterator");
    break;
  case 1:
    return buffer_.firstLine();
    break;
  /* it can be optimized in a reverse iterator context */
  default:
    return buffer_[line_ - 1];
    break;
  }
  return Line();
}
