#pragma once

#include <deque>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

#include <cassert>
#include <cstdint>

#include "rune.h"

struct Buffer {
  using Container = std::vector<Rune>;
  using Line = std::span<const Rune>;

  struct const_reverse_iterator {
    const Line operator * () const;

    const_reverse_iterator & operator ++ () {
      if (0 < line_) {
        --line_;
      }
      return *this;
    }

    bool operator != (const_reverse_iterator & other) const {
      return &buffer_ != &other.buffer_ || line_ != other.line_;
    }

  private:
    const_reverse_iterator(const Buffer & b, const uint32_t l = 0) : buffer_(b), line_(l) { }
    const Buffer & buffer_;
    uint32_t line_ = 0;
    friend class Buffer;
  };

  const_reverse_iterator rbegin() const { return const_reverse_iterator(*this, lines()); }
  const_reverse_iterator rend() const { return const_reverse_iterator(*this); }

  Line firstLine() const {
    assert(!lines_.empty());
    return Line(container_.begin(), lines_[0]);
  }

  Line lastLine() const {
    assert(!lines_.empty());
    return Line(container_.end() - lines_.back(), container_.end());
  }

  Line operator [] (const uint16_t i) const {
    const std::size_t offset = std::accumulate(lines_.begin(), lines_.begin() + i, 0);
    return Line(container_.begin() + offset, lines_[i]);
  }

  std::size_t lines() const { return lines_.size(); }
  void clear();
  void push_back(Rune &&);

private:
  using Lines = std::deque<uint16_t>;

  Container container_;
  Lines lines_{0};

  enum {
    INITIAL = 0,
  } state_ = INITIAL;
};
