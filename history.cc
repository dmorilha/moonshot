#include "history.h"

void History::clear() {
  container_.clear();
  lines_.clear();
  lines_.push_back(0);
}

uint64_t History::pushBack(rune::Rune && rune, uint16_t carriageReturn) {
  bool newLine = false;
  switch (rune.character) {
  case L'\a': // bell 
    return container_.size();
    break;
  case L'\b': // backspace
    if ( ! container_.empty() && ! container_.back().iscontrol()) {
      container_.pop_back();
    }
    return container_.size();
  case L'\n': // new line
    carriageReturn = 0;
    newLine = true;
    break;
  case L'\r': // carriage return
    return container_.size();
  default:
    break;
  }

  if (0 < carriageReturn) {
    container_[container_.size() - 1 - carriageReturn] = std::move(rune);
  } else {
    container_.emplace_back(std::move(rune));
  }

  if (newLine) {
#if 0
    if (0 < cap_ && lines_.size() > cap_) {
      container_.erase(container_.begin(), container_.begin() + lines_[0]);
      lines_.erase(lines_.begin());
    }
#endif
    lines_.push_back(container_.size());
  }

  return container_.size();
}

History::Container::const_iterator History::line(const uint64_t line) const {
  assert(line >= firstLine_);
  assert(lines_.size() > line - firstLine_);
  return container_.begin() + lines_[line];
}

/* does changing a cap invalidate iterators ? */
void History::setCap(const uint32_t cap) {
  cap_ = cap;
}

History::ReverseIterator History::reverseIterator(const uint64_t i) {
  assert(container_.size() >= i);
  ReverseIterator iterator = container_.rbegin();
  iterator += container_.size() - i;
  return iterator;
}

void History::print() {
  for (rune::Rune & rune : container_) {
    if (256 > rune.character) {
      std::cout << static_cast<char>(rune.character) << std::flush;
    } else {
      std::cout << '.' << std::flush;
    }
  }
}

uint64_t History::erase(const uint64_t count) {
  if (0 < count) {
    container_.erase(container_.end() - (count - 1), container_.end());
  }
  return container_.size() - 1;
}
