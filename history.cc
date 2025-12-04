#include <algorithm>

#include "history.h"

void History::emplace(rune::Rune rune) {
  assert(0 < columns_);

  uint8_t stride = 1;

  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\t': // horizontal tab
      stride = 8 - ((last_ % columns_) % 8);
      break;

    default:
      std::cerr << __FILE__ << ":" << __LINE__ << " "
        << static_cast<int>(rune) << " no support for \""
        << static_cast<int>(rune) << "\"" << std::endl;
      assert(!"UNRECHEABLE");
      return;
    }
  }

  ++last_;

  if (0 == last_ % columns_) {
    last_ %= active_.size();
    if (first_ == last_) {
      scrollback();
    }
    ++last_;
  }

  if ( ! static_cast<bool>(active_[last_])) {
    ++active_size_;
  }

  active_[last_] = std::move(rune);

  if (1 < stride) {
    last_ += std::min<uint8_t>(stride - 1,
        columns_ - (last_ % columns_));
    assert(0 < last_ % columns_);
  }

  assert(0 < active_size_);
}

void History::scrollback() {
  assert(0 < columns_);
  assert(0 == first_ % columns_);
  bool new_line = true;
  do {
    for (uint32_t i = 1; i < columns_; ++i) {
      rune::Rune & rune = active_[first_ + i];
      if (static_cast<bool>(rune)) {
        scrollback_.push_back(rune);
        rune = rune::Rune(L'\0');
        --active_size_;
      }
    }
    new_line = active_[first_];
    active_[first_] = rune::Rune(L'\0');
    first_ += columns_;
    first_ %= active_.size();
  } while ( ! new_line);
  scrollback_.push_back(rune::Rune(L'\n'));
  ++scrollback_lines_;
}

History::ReverseIterator History::reverse_iterator(const uint64_t i) {
  assert(scrollback_.size() >= i);
  ReverseIterator iterator = scrollback_.rbegin();
  iterator += scrollback_.size() - i;
  return iterator;
}

void History::resize(uint16_t columns, const uint16_t lines) {
  assert(0 < columns);
  assert(0 < lines);
  ++columns;
#if not NDEBUG
  const uint64_t sizeBefore = size();
#endif
  const uint32_t first = first_;
  Container active(columns * lines);
  std::swap(active_, active);
  std::swap(columns_, columns);
  active_size_ = first_ = last_ = 0;
  if (0 == columns) {
    return;
  }
  assert(0 < sizeBefore);
  for (uint32_t i = 0; active.size() > i; i += columns) {
    const uint32_t line_start = (first + i) % active.size();
    assert(0 == line_start % columns);
    for (uint16_t j = 1; columns > j; ++j) {
      if (active[line_start + j]) {
        emplace(active[line_start + j]);
      }
    }
    if (L'\n' == active[line_start]) {
      carriage_return();
      new_line();
    }
  }
#if not NDEBUG
  if (size() != sizeBefore) {
    std::cerr << "size " << size() << " is different than size before " << sizeBefore << std::endl
      << active_size_ << " " << check_active_size() << std::endl;
  }
  assert(size() == sizeBefore);
#endif
}

uint64_t History::count_lines(History::ReverseIterator & iterator, const History::ReverseIterator & end, const uint64_t limit) const {
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

void History::print() const {
  std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
  uint32_t i = first_;
  while (last_ != i) {
    if (0 == i % columns_) {
      continue;
    }
    if (static_cast<bool>(active_[i])) {
      std::cout << active_[i] << std::flush;
    }
    ++i;
    if (0 == i % columns_) {
      std::cout << '\n' << std::flush;
      i %= active_.size();
    }
  }
}

const rune::Rune & History::at(const uint32_t i) const {
  assert(active_.size() > i);
  return active_[(first_ + i) % active_.size()];
}

const rune::Rune & History::at(const uint16_t column, const uint16_t line) const {
  assert(0 < column);
  assert(0 < line);
  assert(columns_ >= column);
  assert(active_.size() > column + (line - 1) * columns_);
  assert(0 == first_ % columns_);
  const uint32_t index = (first_ + column + (line - 1) * columns_) % active_.size();
  return active_[index];
}

void History::erase_display() {
  std::fill(active_.begin(), active_.end(), rune::Rune(L'\0'));
  active_size_ = first_ = last_ = 0;
}

void History::erase_line_right() {
  assert(0 < columns_);
  uint32_t index = last_;
  if (active_.size() == index) {
    index = 1;
  } else if (0 == index % columns_) {
    ++index;
  }
  for (uint32_t index = last_; 0 != index % columns_; ++index) {
    active_[index] = rune::Rune('\0');
  }
}

void History::erase(const int n) {
  assert(0 < columns_);
  assert(0 < n);
  assert(0 < last_ % columns_);
  assert(0 < last_ + n % columns_);
  const uint32_t dst = last_,
    end = last_ - (last_ % columns_) + columns_,
    src = last_ + n;

  // copies everything from index_ + n to end of line, back to index_,
  // then fill remaining n characters with Rune()
  std::fill(
      std::copy(/* index_ + n */ active_.data() + src,
        /* next line */ active_.data() + end,
        /* index_ */ active_.data() + dst),
      active_.data() + end, rune::Rune(L'\0'));
}

void History::insert(const int n) {
  assert(0 < columns_);
  assert(0 < n);
  assert(n <= columns_ - (last_ % columns_));
  const uint32_t dst = last_ - (last_ % columns_) + columns_,
    end = dst - n,
    src = last_;
  std::copy_backward(active_.data() + src, active_.data() + end, active_.data() + dst); 

  for (uint32_t index = last_; last_ + n > index; ++index) {
    if (!static_cast<bool>(active_[index].character)) {
      ++active_size_;
    }
    active_[index].character = L' ';
  }
}

uint32_t History::check_active_size() const {
  return std::count_if(active_.begin(), active_.end(),
      [](const rune::Rune & r) {
        return L'\0' != r && L'\n' != r;
      });
}

void History::move_cursor_backward(const int n) {
  assert(0 < n);
  assert(n < columns_ - 1);
  const uint32_t column = (last_ % columns_);
  assert(n < column);
  last_ -= n;
  assert(0 < last_ % columns_);
}

void History::move_cursor_down(const int n) {
  assert(0 < n);
  assert(n <= active_.size() / columns_);
  last_ += columns_ * n;
  last_ %= active_.size();
}

void History::move_cursor_forward(const int n) {
  assert(0 < n);
  assert(n < columns_ - 1);
  const uint32_t column = (last_ % columns_);
  assert(n + column < columns_);
  last_ += n;
  assert(0 < last_ % columns_);
}

void History::move_cursor_up(const int n) {
  assert(0 < n);
  assert(n <= active_.size() / columns_);
  last_ -= columns_ * n;
  last_ %= active_.size();
}

void History::move_cursor(const int column, const int line) {
  assert(0 < column);
  assert(columns_ >= column);
  assert(0 < line);
  assert(active_.size() / columns_ >= line);
  last_ = column + (line - 1) * columns_;
  assert(0 < last_ % columns_);
}

void History::carriage_return() {
  assert(0 < columns_);
  last_ = last_ - (last_ % columns_);
}

void History::new_line() {
  active_[last_ - (last_ % columns_)] = rune::Rune(L'\n');
  last_ = (last_ + columns_) % active_.size();
  if (last_ >= first_ && last_ < first_ + columns_) {
    scrollback();
  }
}

uint64_t History::size() const {
  return scrollback_size() + active_size_;
}
