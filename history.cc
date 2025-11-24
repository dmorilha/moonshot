#include <algorithm>

#include "history.h"

void History::pushBack(rune::Rune && rune) {
  bool newLine = false;
  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\b': // backspace
      --index_;
      return;
    case L'\n': // new line
      newLine = true;
      break;
    case L'\r': // carriage return
      carriage_return_ = true;
      return;
    default:
      std::cerr << __FILE__ << ":" << __LINE__ << " "
        << static_cast<int>(rune) << " no support for \""
        << static_cast<int>(rune) << "\"" << std::endl;
      return;
    }
  }

  if (newLine) {
    active_[index_] = L'\n';
    index_ += columns_ - (index_ % columns_);
  } else {
    if (carriage_return_) {
      const int16_t columns = index_ % columns_;
      index_ -= columns;
    }
    active_[index_] = std::move(rune);
    index_ = (index_ + 1) % active_.size();
  }
  index_ %= active_.size();
  carriage_return_ = false;
  if (index_ == start_) {
    for (int i = 0; i < columns_; ++i) {
      rune::Rune & rune = active_[start_ + i];
      if (rune) {
        scrollback_.push_back(rune);
        rune = rune::Rune();
      }
    }
    start_ = (start_ + columns_) % active_.size();
    scrollback_lines_ += 1;
  }
  return;
}

History::ReverseIterator History::reverseIterator(const uint64_t i) {
  assert(scrollback_.size() >= i);
  ReverseIterator iterator = scrollback_.rbegin();
  iterator += scrollback_.size() - i;
  return iterator;
}

void History::resize(const uint16_t columns, const uint16_t lines) {
  assert(0 < columns);
  assert(0 < lines);
#if DEBUG
  const uint64_t sizeBefore = size();
#endif
  const uint16_t oldColumns = columns_;
  const uint32_t end = index_;
  uint32_t index = start_;
  Container active(columns * lines);
  std::swap(active_, active);
  columns_ = columns;
  index_ = start_ = 0;
  carriage_return_ = false;
  if (0 == oldColumns) {
    return;
  }
#if DEBUG
  assert(0 < sizeBefore);
#endif
  while (end != index) {
    assert(static_cast<bool>(active[index]));
    const bool newLine = L'\n' == active[index];
    pushBack(std::move(active[index]));
    if (newLine) {
      index = index - (index % oldColumns) + oldColumns;
    } else {
      ++index;
    }
    index %= active.size();
  }
#if DEBUG
  assert(sizeBefore == size());
#endif
}

uint64_t History::countLines(History::ReverseIterator & iterator, const History::ReverseIterator & end, const uint64_t limit) const {
  uint64_t lines = 0; // lines returns zero, only when iterator equals end from the start
  History::ReverseIterator lineBegin = std::find(iterator, end, L'\n');
  while (end != iterator && (0 == limit || limit >= lines)) {
    ++lines;
    uint32_t cursor_column = 1;
    for (History::ReverseIterator line_iterator = lineBegin - 1;
        iterator <= line_iterator; --line_iterator) {
      uint32_t columns = 1;
      if (line_iterator->iscontrol()) {
        switch (line_iterator->character) {
        case L'\n': // new line
          continue;
        case L'\t': // horizontal tab
          columns = 8 - (cursor_column % 8);
          break;
        default:
          std::cerr << static_cast<int>(line_iterator->character) << std::endl;
          assert(!"UNIMPLEMENTED");
          break;
        }
      }
      const bool wrap = columns_ < cursor_column;
      if (wrap) {
        cursor_column = 1;
        ++lines;
        if (0 < limit && limit < lines) {
          iterator = line_iterator;
          return limit;
        }
      }
      cursor_column += columns;
    }
    if (limit < lines) {
      return limit;
    }
    iterator = lineBegin;
    if (end != lineBegin) {
      ++lineBegin;
      lineBegin = std::find(lineBegin, end, L'\n');
    }
  }
  return lines;
}

uint16_t History::line() const {
  assert(0 < columns_);
  assert(0 == start_ % columns_);
  uint16_t bottom = 0;
  if (index_ < start_) {
    bottom = active_.size() - start_ / columns_;
  } else {
    return index_ / columns_ - start_ / columns_;
  }
  return index_ / columns_ + bottom /* if any */ ;
}

uint32_t History::active_size() const {
  return std::count_if(active_.begin(), active_.end(),
      [](const rune::Rune & rune){ return static_cast<bool>(rune); });
}

void History::print() const {
  std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
  uint32_t i = start_;
  while (index_ != i) {
    assert(static_cast<bool>(active_[i]));
    const bool newLine = L'\n' == active_[i];
    std::cout << active_[i] << std::flush;
    if (newLine) {
      i = i - (i % columns_) + columns_;
    } else {
      ++i;
      if (0 == i % columns_) {
        std::cout << '\n' << std::flush;
      }
    }
    i %= active_.size();
  }
}

const rune::Rune & History::active(const uint32_t i) const {
  assert(active_.size() > i);
  return active_[(start_ + i) % active_.size()];
}
