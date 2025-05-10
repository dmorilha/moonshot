#pragma once

struct Color {
  float red = 1.f;
  float green = 0.f;
  float blue = 0.f;
  float alpha = 1.f;
  operator float* () const { return (float *)this; }
};

struct Char {
  bool hasBackgroundColor = false;
  bool hasForegroundColor = false;
  Color backgroundColor;
  Color foregroundColor;
  char character;
  const Char & operator = (const char c) { character = c; return *this; }
  bool operator == (const char c) const { return character == c; }
};
