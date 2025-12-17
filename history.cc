// Copyright Daniel Morilha 2025

#include <algorithm>

#include "history.h"

#define DEBUG_ACTIVE_SIZE 0

void History::emplace(rune::Rune rune) {
  assert(0 < columns_);

  uint8_t stride = 1;

  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\n':
      assert(!"UNRECHEABLE");
      return;

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

  if (0 < long_transaction_) {
    ++long_transaction_;
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
#if DEBUG_ACTIVE_SIZE
    std::cout << __func__ << " " << __LINE__ << " ++active_size_ = " << ++active_size_ << std::endl;
#else
    ++active_size_;
#endif
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
  if (is_scrollback_disabled()) {
    first_ = (first_ + columns_) % active_.size();
    return;
  }
  assert(0 < columns_);
  assert(0 == first_ % columns_);
  bool new_line = true;
  uint32_t counter = 0;
  do {
    new_line = L'\n' == active_[first_];
    if (new_line) {
#if DEBUG_ACTIVE_SIZE
      std::cout << __func__ << " " << __LINE__ << " --active_size_ = " << --active_size_ << std::endl;
#else
      --active_size_;
#endif
      active_[first_] = rune::Rune(L'\0');
    }
    for (uint32_t i = 1; i < columns_; ++i) {
      rune::Rune & rune = active_[first_ + i];
      if (static_cast<bool>(rune)) {
        scrollback_.push_back(rune);
        rune = rune::Rune(L'\0');
#if DEBUG_ACTIVE_SIZE
        std::cout << __func__ << " " << __LINE__ << " --active_size_ = " << --active_size_ << std::endl;
#else
        --active_size_;
#endif
      }
    }
    first_ = (first_ + columns_) % active_.size();
    ++counter;
    if (active_.size() == counter) {
      break;
    }
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
  if (active_.size() == columns * lines) {
    return; // no resize if new size is the same.
  }
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
      << active_size_ << " " << check_size(active_) << std::endl;
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

void History::print_active() const {
  std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
  uint32_t i = first_;
  bool new_line = L'\n' == active_[i];
  ++i;
  while (first_ != i) {
    if (0 == i % columns_) {
      if (new_line) {
        std::cout << '\n' << std::flush;
      }
      i %= active_.size();
      new_line = L'\n' == active_[i];
      ++i;
    }
    if (static_cast<bool>(active_[i])) {
      std::cout << active_[i] << std::flush;
    }
    i = (i + 1) % active_.size();
  }
  if (new_line) {
    std::cout << '\n' << std::flush;
  }
}

void History::print_scrollback() const {
  std::cout << __FILE__ << ":" << __LINE__ << " " << __func__ << std::endl;
  for (const auto & rune : scrollback_) {
    if (static_cast<bool>(rune)) {
      std::cout << rune << std::flush;
    }
  }
  std::cout << std::endl;
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
  if (active_.size() <= index) {
    index = 1;
  } else if (0 == index % columns_) {
    ++index;
  }
  for (; 0 != index % columns_; ++index) {
    if (static_cast<bool>(active_[index])) {
#if DEBUG_ACTIVE_SIZE
      std::cout << __func__ << " " << __LINE__ << " --active_size_ = " << --active_size_ << std::endl;
#else
      --active_size_;
#endif
    }
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
    if ( ! static_cast<bool>(active_[index].character)) {
#if DEBUG_ACTIVE_SIZE
      std::cout << __func__ << " " << __LINE__ << " ++active_size_ = " << ++active_size_ << std::endl;
#else
      ++active_size_;
#endif
    }
    active_[index].character = L' ';
  }
}

uint32_t History::check_size(const Container & c) const {
  return std::count_if(c.begin(), c.end(),
      [](const rune::Rune & r) { return static_cast<bool>(r); });
}

void History::move_cursor_backward(const int n) {
  assert(0 < n);
  assert(n < columns_ - 1);
  const uint32_t column = (last_ % columns_);
  if (1 == column) {
    return;
  }
  assert(n < column);
  last_ -= n;
  assert(0 <= last_);
  assert(active_.size() > last_);
  last_ %= active_.size();
  if (0 == last_ % columns_) {
    --last_;
  }
}

void History::move_cursor_down(const int n) {
  assert(0 < n);
  assert(n <= active_.size() / columns_);
  last_ += columns_ * n - 1;
  last_ %= active_.size();
  if (0 == last_ % columns_) {
    --last_;
  }
}

void History::move_cursor_forward(const int n) {
  assert(0 < n);
  assert(n < columns_ - 1);
  const uint32_t column = (last_ % columns_);
  assert(n + column < columns_);
  last_ += n;
  assert(0 <= last_);
  assert(active_.size() > last_);
  last_ %= active_.size();
  if (0 == last_ % columns_) {
    --last_;
  }
}

void History::move_cursor_up(const int n) {
  assert(0 < n);
  assert(n <= active_.size() / columns_);
  last_ -= columns_ * n - 1;
  last_ %= active_.size();
  if (0 == last_ % columns_) {
    --last_;
  }
}

void History::move_cursor(const int column, const int line) {
  assert(0 < column);
  assert(columns_ >= column);
  assert(0 < line);
  assert(active_.size() / columns_ >= line);
  last_ = first_ + (line - 1) * columns_ + column - 0;
  last_ %= active_.size();
  if (0 == last_ % columns_) {
    --last_;
  }
}

void History::carriage_return() {
  assert(0 < columns_);
  if (-1 == last_) {
    return;
  }
  assert(active_.size() > last_);
  last_ = last_ - (last_ % columns_);
}

void History::new_line() {
  rune::Rune & rune = active_[last_ - (last_ % columns_)];
  last_ = (last_ + columns_) % active_.size();
  if (is_scrollback_enabled()) {
    assert( ! static_cast<bool>(rune));
#if DEBUG_ACTIVE_SIZE
    std::cout << __func__ << " " << __LINE__ << " ++active_size_ = " << ++active_size_ << std::endl;
#else
    ++active_size_;
#endif
  } else /* scrollback is disabled */ {
    if ( ! static_cast<bool>(rune)) {
#if DEBUG_ACTIVE_SIZE
      std::cout << __func__ << " " << __LINE__ << " ++active_size_ = " << ++active_size_ << std::endl;
#else
    ++active_size_;
#endif
    }
  }
  rune = rune::Rune(L'\n');
  if (last_ >= first_ && last_ < first_ + columns_) {
    scrollback();
  }
}

uint64_t History::size() const {
  return scrollback_size() + active_size_;
}

std::pair<uint16_t, uint16_t> History::get_cursor() const {
  std::pair<uint16_t, uint16_t> cursor{0, 0};
  uint32_t position = 0;
  if (last_ < first_) {
    position = active_.size() - (first_ - last_);
  } else {
    position = last_ - first_;
  }
  cursor.first = position % columns_;
  cursor.second = 1 + (position / columns_);
  return cursor;
}

void History::alternative(const bool mode) {
  if (mode) {
    saved_ = Container(active_.size());
    std::swap(saved_, active_);
    saved_columns_ = columns_;
    saved_first_ = first_;
    saved_last_ = last_;
    saved_size_ = active_size_;
    active_size_ = first_ = last_ = 0;
  } else {
    std::swap(saved_, active_);
    std::swap(columns_, saved_columns_);
    assert(0 == saved_.size() % saved_columns_);
#if 1
    assert(columns_ == saved_columns_);
#else
    const int16_t lines = saved_.size() / saved_columns_;
    if (active_.size() != saved_columns_ * lines) {
      resize(saved_columns_ - 1, lines);
    }
#endif
    first_ = saved_first_;
    last_ = saved_last_;
    active_size_ = saved_size_;
    saved_.clear();
    saved_columns_ = saved_first_ = saved_last_ = saved_size_ = 0;
  }
}

void History::reverse_line_feed() {
  assert(0 < columns_);
  const bool pull_from_scrollback = (first_ == last_ - (last_ % columns_));

  if (pull_from_scrollback) {
    assert(0 == first_ % columns_);

    if ( ! static_cast<bool>(active_[first_])) {
#if DEBUG_ACTIVE_SIZE
      std::cout << __func__ << " " << __LINE__ << " ++active_size_ = " << -++active_size_ << std::endl;
#else
      ++active_size_;
#endif
      active_[first_] = rune::Rune(L'\n');
    }

    // moves first_ one line up.
    if (0 == first_) {
      first_ = active_.size() - columns_;
    } else {
      first_ -= columns_;
    }
    assert(0 == first_ % columns_);

    if ( ! static_cast<bool>(active_[first_])) {
#if DEBUG_ACTIVE_SIZE
      std::cout << __func__ << " " << __LINE__ << " ++active_size_ = " << -++active_size_ << std::endl;
#else
      ++active_size_;
#endif
      active_[first_] = rune::Rune(L'\n');
    }

    if ( ! scrollback_.empty()) {
      assert(L'\n' == scrollback_.back());
      scrollback_.pop_back();
    }

    Container::const_reverse_iterator back_iterator = scrollback_.crbegin();
    for (uint16_t i = 1; columns_ > i && scrollback_.crend() != back_iterator; ++i, ++back_iterator) {
      if (L'\n' == *back_iterator) {
        break;
      }
    }
    Container::const_reverse_iterator iterator = back_iterator;
    if (scrollback_.crend() != iterator && L'\n' == *iterator) {
      --iterator;
    }
    for (uint32_t index = 1; columns_ > index; ++index) {
      const bool has_rune = static_cast<bool>(active_[first_ + index]);
      if (scrollback_.crbegin() < iterator && scrollback_.crend() > iterator) {
        active_[first_ + index] = *iterator;
        --iterator;
        if ( ! has_rune) {
#if DEBUG_ACTIVE_SIZE
          std::cout << __func__ << " " << __LINE__ << " ++active_size_ = " << ++active_size_ << std::endl;
#else
          ++active_size_;
#endif
        }
      } else if (has_rune) {
#if DEBUG_ACTIVE_SIZE
        std::cout << __func__ << " " << __LINE__ << " --active_size_ = " << --active_size_ << std::endl;
#else
        --active_size_;
#endif
        active_[first_ + index] = rune::Rune(L'\0');
      }
    }
    {
      const std::size_t distance = std::distance(scrollback_.crbegin(), back_iterator);
      assert(scrollback_.size() > distance);
      for (uint16_t i = 0; distance > i; ++i) {
        scrollback_.pop_back();
      }
    }
  }

  if ( ! scrollback_.empty() && L'\n' != scrollback_.back()) {
    scrollback_.emplace_back(rune::Rune{L'\n'});
  }

  // moves last_ back one line + one positions
  if (columns_ > last_) {
    last_ = active_.size() - columns_ + (last_ % columns_);
  } else {
    last_ -= columns_;
  }
  --last_;
}
