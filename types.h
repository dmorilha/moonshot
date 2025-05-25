#pragma once

#include <ostream>

#include <cstdint>

struct Color {
  float red = 1.f;
  float green = 1.f;
  float blue = 1.f;
  float alpha = 1.f;
  operator const float * () const { return reinterpret_cast< const float * >(this); }
};

struct Rectangle {
  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;;
  operator const int * () const { return reinterpret_cast< const int * >(this); }
  friend std::ostream & operator << (std::ostream &, const Rectangle &);
};
