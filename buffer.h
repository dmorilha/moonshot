#pragma once

#include <vector>

#include <cassert>

#include "char.h"

// create iterators
struct Buffer {
  struct Iterator {
    Iterator() = default;
    Iterator(Buffer * b) : buffer_(b) { }
    bool operator == (const Iterator & other) const { return buffer_ == other.buffer_ && column_ == other.column_ && row_ == other.row_; }
    Iterator & operator ++ ();
    Char & operator * () const;
  private:
    Buffer * buffer_ = nullptr;
    std::size_t column_ = 0;
    std::size_t row_ = 0;
  };

  using Row = std::vector<Char>;

  Buffer();

  Iterator begin() { return Iterator(this); }
  Iterator end() { return Iterator(); }
  void clear();
  void push_back(const Char &);
  void set_columns(std::size_t c) { columns_ = c; }

private:
  std::size_t columns_ = 0;
  std::vector< Row > buffer_;
};
