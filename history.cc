#include <algorithm>

#include "history.h"

void History::emplace(rune::Rune && rune) {
  bool newLine = false;
  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\0': // null
      assert(!"UNRECHEABLE");
      return;
    case L'\a': // bell
      return;
    case L'\b': // backspace
      --index_;
      ++returned_;
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
  if (carriage_return_ && ! newLine) {
    const uint32_t value = index_ % columns_;
    index_ -= value;
    returned_ += value;
  }
  if ( ! static_cast<bool>(active_[index_])) {
    ++active_size_;
  }
  active_[index_] = rune;
  if (newLine) {
    const uint32_t value = columns_ - (index_ % columns_);
    index_ += value;
    returned_ = 0;
  } else {
    ++index_;
    if (0 < returned_) {
      --returned_;
    }
  }
  index_ %= active_.size();
  carriage_return_ = false;
  if (index_ == start_) {
    for (uint32_t i = 0; i < columns_; ++i) {
      rune::Rune & rune = active_[start_ + i];
      if (rune) {
        scrollback_.push_back(rune);
        rune = rune::Rune();
        --active_size_;
      }
    }
    start_ = (start_ + columns_) % active_.size();
    scrollback_lines_ += 1;
  }
  assert(0 < active_size_);
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
#if not NDEBUG
  const uint64_t sizeBefore = size();
#endif
  const uint16_t oldColumns = columns_;
  const uint32_t end = index_;
  uint32_t index = start_;
  Container active(columns * lines);
  std::swap(active_, active);
  columns_ = columns;
  active_size_ = index_ = start_ = 0;
  carriage_return_ = false;
  if (0 == oldColumns) {
    return;
  }
  assert(0 < sizeBefore);
  while (end != index) {
    assert(static_cast<bool>(active[index]));
    const bool newLine = L'\n' == active[index];
    emplace(std::move(active[index]));
    if (newLine) {
      index = index - (index % oldColumns) + oldColumns;
    } else {
      ++index;
    }
    index %= active.size();
  }
#if not NDEBUG
  if (size() != sizeBefore) {
    std::cerr << "size " << size() << " is different than size before " << sizeBefore << std::endl
      << active_size_ << " " << check_active_size() << std::endl;
  }
  assert(size() == sizeBefore);
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

const rune::Rune & History::at(const uint32_t i) const {
  assert(active_.size() > i);
  return active_[(start_ + i) % active_.size()];
}

const rune::Rune & History::next(const uint32_t i) const {
  assert(active_.size() > i);
  return active_[(index_ + i) % active_.size()];
}

void History::clear() {
  assert(0 < columns_);
  assert(0 == start_ % columns_);
#if not NDEBUG
  const uint64_t sizeBefore = size();
#endif
  const uint32_t end = index_ - (index_ % columns_);
  for (; end != start_; start_ = (start_ + 1) % active_.size()) {
    rune::Rune & rune = active_[start_];
    if (rune) {
      scrollback_.push_back(rune);
      rune = rune::Rune();
      --active_size_;
    }
  }
  assert(0 == start_ % columns_);
  assert(sizeBefore == size());
}

void History::erase_line_right() {
  assert(0 < columns_);
  // dassert(0 < returned_);
  const uint32_t end = index_ - (index_ % columns_) + columns_;
  for (uint32_t index = index_; end > index; ++index) {
    rune::Rune & rune = active_[index];
    if (rune) {
      rune = rune::Rune();
      --active_size_;
    }
  }
  returned_ = 0;
}

void History::erase(const int n) {
  assert(0 < columns_);
  assert(0 < n);
  assert(n <= returned_);
  const uint32_t dst = index_,
    end = index_ - (index_ % columns_) + columns_,
    src = index_ + n;

  // copies everything from index_ + n to end of line, back to index_,
  // then fill remaining n characters with Rune()
  std::fill(
      std::copy(/* index_ + n */ active_.data() + src,
        /* next line */ active_.data() + end,
        /* index_ */ active_.data() + dst),
      active_.data() + end, rune::Rune());

  returned_ -= n;
}

void History::insert(const int n) {
  assert(0 < columns_);
  assert(0 < n);
  assert(n <= columns_ - (index_ % columns_));
  const uint32_t dst = index_ - (index_ % columns_) + columns_,
    end = dst - n,
    src = index_;
  std::copy_backward(active_.data() + src, active_.data() + end, active_.data() + dst); 
  for (uint32_t index = index_; index_ + n > index; ++index) {
    assert(static_cast<bool>(active_[index].character));
    active_[index].character = L' ';
  }

  returned_ += n;
}

uint32_t History::check_active_size() const {
  return std::count_if(active_.begin(), active_.end(), [](const rune::Rune & r) { return static_cast<bool>(r); });
}

void History::move_cursor_backward(const int n) {
  assert(0 < n);
  assert(n < columns_);
  assert(index_ - (index_ % columns_) <= index_ - n);
  index_ -= n;
  returned_ = std::max<uint32_t>(returned_ - n, 0);
}

void History::move_cursor_down(const int n) {
  assert(0 < n);
  assert(n <= active_.size() / columns_);
  index_ = (index_ + n * columns_) % active_.size();
  returned_ = 0;
}

void History::move_cursor_forward(const int n) {
  assert(0 < n);
  assert(n < columns_);
  assert(columns_ >= (index_ % columns_) + n);
  index_ += n;
  returned_ = std::max<uint32_t>(returned_ - n, 0);
}

void History::move_cursor_up(const int n) {
  assert(0 < n);
  assert(n <= active_.size() / columns_);
  index_ = (index_ - n * columns_) % active_.size();
  returned_ = 0;
}
