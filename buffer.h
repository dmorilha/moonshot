#pragma once

#include <vector>

#include <cassert>

#include "char.h"

/* need to come with some type of wraping */
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

private:
  Rows rows_;
};
