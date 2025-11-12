#pragma once

#include <deque>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

#include <cassert>
#include <cstdint>

#include "rune.h"

struct History {
  using Container = std::vector<rune::Rune>;
  using Iterator = Container::const_iterator;
  using ReverseIterator = Container::const_reverse_iterator;

  auto clear() -> void;
  auto erase(const uint64_t) -> uint64_t;
  auto line(const uint64_t) const -> Iterator;
  auto lines() const -> std::size_t { return lines_.size(); }
  auto operator [] (const uint64_t i) const { return container_.operator[](i); }
  auto print() -> void;
  auto pushBack(rune::Rune &&, uint16_t cr = 0) -> uint64_t;
  auto rbegin() const -> ReverseIterator { return container_.rbegin(); }
  auto rend() const -> ReverseIterator { return container_.rend(); }
  auto reverseIterator(const uint64_t) -> ReverseIterator;
  auto setCap(const uint32_t) -> void;
  auto size() const -> uint64_t { return container_.size(); }

private:
  using Lines = std::deque<uint64_t>;
  Container container_;
  Lines lines_ = { /* first line always starts at position */ 0 };
  uint64_t cap_ = 0;
  uint64_t firstLine_ = 0;
  uint64_t offset_ = 0;
};
