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

namespace color {
  constexpr static Color black{.red = 0.f, .green = 0.f, .blue = 0.f, .alpha = 1.f};
  constexpr static Color blue{.red = 0.f, .green = 0.f, .blue = 1.f, .alpha = 1.f};
  constexpr static Color green{.red = 0.f, .green = 1.f, .blue = 0.f, .alpha = 1.f};
  constexpr static Color red{.red = 1.f, .green = 0.f, .blue = 0.f, .alpha = 1.f};
  constexpr static Color white{.red = 1.f, .green = 1.f, .blue = 1.f, .alpha = 1.f};
} //end of namespace color

struct Rectangle {
  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;;
  operator const int * () const { return reinterpret_cast< const int * >(this); }
  friend std::ostream & operator << (std::ostream &, const Rectangle &);
};
