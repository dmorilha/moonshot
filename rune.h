#pragma once

#include <string>

#include <cwchar>

struct Color {
  float red = 1.f;
  float green = 1.f;
  float blue = 1.f;
  float alpha = 1.f;
  operator const float * () const { return reinterpret_cast< const float * >(this); }
};

struct Rune {
  bool hasBackgroundColor = false;
  bool hasForegroundColor = false;
  Color backgroundColor;
  Color foregroundColor;
  wchar_t character;

  enum {
    REGULAR,
    BOLD,
    ITALIC,
    BOLD_AND_ITALIC,
  } style = REGULAR;

  const Rune & operator = (const char c) { character = c; return *this; }
  bool operator == (const char c) const { return character == c; }
  operator std::string() const;

  friend std::ostream & operator << (std::ostream &, const Rune &);
};
