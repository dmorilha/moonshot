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

const Buffer::Line Buffer::const_reverse_iterator::operator * () const {
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
      return Line(iterator + offset, iterator + currentOffset_);
    }
    break;
  }
  return Line();
}
