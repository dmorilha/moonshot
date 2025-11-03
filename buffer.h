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
  using Container = std::vector<rune::Rune>;
  using Span = std::span<const rune::Rune>;

  struct const_reverse_iterator {
    const Span operator * () const;

    const_reverse_iterator & operator ++ () {
      if (0 < line_) {
        --line_;
      }
      currentOffset_ -= buffer_.lines_[line_];
      return *this;
    }

    bool operator != (const_reverse_iterator & other) const {
      return &buffer_ != &other.buffer_ || line_ != other.line_;
    }

  private:
    const_reverse_iterator(const Buffer &, const uint32_t l = 0);
    const Buffer & buffer_;
    uint32_t line_ = 0;
    std::size_t currentOffset_;

    friend class Buffer;
  };

  const_reverse_iterator rbegin() const { return const_reverse_iterator(*this, lines()); }

  const_reverse_iterator rend() const { return const_reverse_iterator(*this); }

  Span firstLine() const {
    assert(!lines_.empty());
    return Span(container_.begin(), lines_[0]);
  }

  Span lastLine() const {
    assert(!lines_.empty());
    return Span(container_.end() - lines_.back(), container_.end());
  }

  void setCap(const uint32_t);
  std::size_t lines() const { return lines_.size(); }
  void clear();
  void pushBack(rune::Rune &&);
  rune::Rune get(const uint32_t, const uint16_t);

private:
  using Lines = std::deque<uint16_t>;

  Container container_;
  Lines lines_ = {0};
  uint32_t cap_ = 0;

  enum {
    INITIAL = 0,
  } state_ = INITIAL;
};
