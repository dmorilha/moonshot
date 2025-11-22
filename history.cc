#include "history.h"

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
    ++lines_;
  }

  return container_.size();
}

History::ReverseIterator History::reverseIterator(const uint64_t i) {
  assert(container_.size() >= i);
  ReverseIterator iterator = container_.rbegin();
  iterator += container_.size() - i;
  return iterator;
}

uint64_t History::erase(const uint64_t count) {
  if (0 < count) {
    container_.erase(container_.end() - (count - 1), container_.end());
  }
  return container_.size() - 1;
}
