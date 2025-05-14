#pragma once

#include <vector>

#include <cassert>

#include "char.h"

struct Buffer {
  // not the most efficient storage
  using Row = std::vector< Char >;
  using Rows = std::vector< Row >;

  Buffer();

  Rows::const_reverse_iterator begin() const { return rows_.rbegin(); }
  Rows::const_reverse_iterator end() const { return rows_.rend(); }
  std::size_t rows() const { return rows_.size(); }
  void clear();
  void push_back(const Char &);
  void set_columns(std::size_t c) { columns_ = c; }

private:
  std::size_t columns_ = 0;
  Rows rows_;
};
