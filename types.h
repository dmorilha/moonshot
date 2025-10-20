#pragma once

#include <iostream>
#include <limits>
#include <ostream>

#include <cstdint>

struct Color {
  float red = 0.f;
  float green = 0.f;
  float blue = 0.f;
  float alpha = 1.f;
  operator const float * () const { return reinterpret_cast< const float * >(this); }
};

namespace color {
  constexpr static Color black{.red = 0.f, .green = 0.f, .blue = 0.f, .alpha = 1.f};
  constexpr static Color blue{.red = 0.f, .green = 0.f, .blue = 1.f, .alpha = 1.f};
  constexpr static Color cyan{.red = 0.f, .green = 1.f, .blue = 1.f, .alpha = 1.f};
  constexpr static Color green{.red = 0.f, .green = 1.f, .blue = 0.f, .alpha = 1.f};
  constexpr static Color magenta{.red = 1.f, .green = 0.f, .blue = 1.f, .alpha = 1.f};
  constexpr static Color red{.red = 1.f, .green = 0.f, .blue = 0.f, .alpha = 1.f};
  constexpr static Color white{.red = 1.f, .green = 1.f, .blue = 1.f, .alpha = 1.f};
  constexpr static Color yellow{.red = 1.f, .green = 0.8f, .blue = 0.f, .alpha = 1.f};
} //end of namespace color

struct Rectangle {
  // the order of these fields cannot be changed.
  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;;
  operator const int * () const { return reinterpret_cast< const int * >(this); }
  friend std::ostream & operator << (std::ostream &, const Rectangle &);
};

struct Rectangle_Y {
  int32_t x = 0;
  uint64_t y = 0;
  int32_t width = 0;
  int32_t height = 0;;
  operator Rectangle () const {
    if (std::numeric_limits<int32_t>::max() < y) {
      std::cerr << "narrowing " << y << " to an int32_t" << std::endl;
    }
    return Rectangle{
      .x = x,
      .y = static_cast<int32_t>(y),
      .width = width,
      .height = height,
    };
  }
};
