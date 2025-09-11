#include "buffer.h"

void Buffer::clear() {
  container_.clear();
  lines_.clear();
  lines_.push_back(0);
  state_ = INITIAL;
}

void Buffer::pushBack(rune::Rune && rune) {
  bool newLine = false;
  if (rune.iscontrol()) {
    switch (rune.character) {
    case L'\a': // BELL
      // what should we do?
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
    if (0 < cap_ && lines_.size() > cap_) {
      container_.erase(container_.begin(), container_.begin() + lines_[0]);
      lines_.erase(lines_.begin());
    }
    lines_.push_back(0);
    state_ = INITIAL;
  }
}

/* does changing a cap invalidate iterators ? */
void Buffer::setCap(const uint32_t c) {
  cap_ = c;
  if (lines_.size() > cap_) {
    const std::size_t length = lines_[lines_.size() - cap_ - 1];
    container_.erase(container_.begin(), container_.begin() + length);
  }
}

Buffer::const_reverse_iterator::const_reverse_iterator(const Buffer & b, const uint32_t l) : buffer_(b), line_(l) {
  const auto iterator = buffer_.lines_.begin();
  currentOffset_ = std::accumulate(iterator, iterator + line_, 0);
}

const Buffer::Span Buffer::const_reverse_iterator::operator * () const {
  const std::size_t size = buffer_.container_.size();
  switch (line_) {
  case 0:
    throw std::runtime_error("cannot dereference end iterator");
    break;
  case 1:
    return buffer_.firstLine();
    break;
  default:
    {
      const std::size_t offset = currentOffset_ - buffer_.lines_[line_ - 1];
      const auto iterator = buffer_.container_.begin();
      return Span(iterator + offset, iterator + currentOffset_);
    }
    break;
  }
  return Span();
}

Buffer::Span Buffer::difference() {
  Span result;
  uint64_t mark = container_.size();
  std::swap(mark, mark_);
  result = Span(container_.begin() + mark, container_.begin() + mark_);
  return result;
}

rune::Rune Buffer::get(const uint32_t line, const uint16_t column) {
  assert(0 < column);
  assert(0 < line);
  if (line <= lines_.size()) {
    const auto linesIterator = lines_.begin();
    const uint64_t offset = std::accumulate(linesIterator, linesIterator + line - 1, 0);
    const uint16_t size = lines_[line - 1];

    Container::const_iterator iterator = container_.cbegin() + offset;
    for (uint16_t i = 1, j = 1; j <= size; ++i, ++j, ++iterator) {
      assert(container_.cend() != iterator);
      if (column == i) {
        return *iterator;
      }
      if (L'\t' == *iterator) {
        const uint16_t tab = 8 - (i % 8);
        if (column > i && column <= i + tab) {
          return *iterator;
        }
        i += tab;
      }
    }
  }
  return rune::Rune('\0');
}
