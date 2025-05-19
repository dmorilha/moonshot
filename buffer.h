#pragma once

#include <vector>

#include <cassert>

#include "rune.h"

struct Buffer {
  // not the most efficient storage
  using Line = std::vector< Rune >;
  using Lines = std::vector< Line >;

  Buffer();

  Lines::const_reverse_iterator begin() const { return lines_.rbegin(); }
  Lines::const_reverse_iterator end() const { return lines_.rend(); }
  std::size_t lines() const { return lines_.size(); }
  void clear();
  void push_back(const Rune &);

private:
  Lines lines_;
};
