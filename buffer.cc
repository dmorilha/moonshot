#include <iostream>

#include "buffer.h"

Buffer::Buffer() {
  buffer_.emplace_back(Row{});
}

void Buffer::clear() {
  for (Row & row : buffer_) {
    row.clear();
  }
  buffer_.clear();
}

void Buffer::push_back(const Char & c) {
  bool newLine = false;
  switch (c.character) {
    case '\a': // NEW LINE
      // what should we do?
      return;
      break;

    case '\b': // NEW LINE
      {
        if ( ! buffer_.back().empty()) {
          buffer_.back().erase(buffer_.back().end());
        }
        return;
      }
      break;

    case '\n': // NEW LINE
      newLine = true;
      break;

    default:
      break;
  }

  assert(0 < columns_);
  if (columns_ <= buffer_.back().size()) {
    newLine = true;
  }

  buffer_.back().push_back(c);

  if (newLine) {
    buffer_.emplace_back(Row{});
  }
}

Char & Buffer::Iterator::operator * () const {
  assert(nullptr != buffer_);
  assert(buffer_->buffer_.size() > row_);
  assert(buffer_->buffer_[row_].size() > column_);
  return buffer_->buffer_[row_][column_];
}

Buffer::Iterator & Buffer::Iterator::operator ++ () {
  ++column_;
  if (nullptr != buffer_) {
    if (buffer_->buffer_[row_].size() == column_) {
      column_ = 0;
      ++row_;
    }
  }
  if (buffer_->buffer_.size() <= row_) {
    buffer_ = nullptr;
    column_ = 0;
    row_ = 0;
  }
  return *this;
}
