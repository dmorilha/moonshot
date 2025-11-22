#pragma once

#include <deque>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

#include <cassert>
#include <cstdint>

#include "rune.h"

class History {
  using Container = std::vector<rune::Rune>;

public:
  using Iterator = Container::const_iterator;
  using ReverseIterator = Container::const_reverse_iterator;

  auto erase(const uint64_t) -> uint64_t;
  auto lines() const -> std::size_t { return lines_; }
  auto operator [] (const uint64_t i) const { return container_.operator[](i); }
  auto pushBack(rune::Rune &&, uint16_t cr = 0) -> uint64_t;
  auto rbegin() const -> ReverseIterator { return container_.rbegin(); }
  auto rend() const -> ReverseIterator { return container_.rend(); }
  auto reverseIterator(const uint64_t) -> ReverseIterator;
  auto size() const -> uint64_t { return container_.size(); }

private:
  Container container_;
  uint64_t lines_ = 0;
};
