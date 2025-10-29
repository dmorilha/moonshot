#pragma once

#include <string>

#include <cwchar>

#include "types.h"

namespace rune {

enum class Style {
  REGULAR,
  BOLD,
  ITALIC,
  BOLD_AND_ITALIC,
};

struct RuneFactory;

struct Rune {
  Color backgroundColor;
  Color foregroundColor;
  Style style = Style::REGULAR;
  bool hasBackgroundColor = false;
  bool hasForegroundColor = false;
  wchar_t character;

  Rune() = default;
  Rune(const wchar_t c) : character(c) { }

  bool isalphanumeric() const { return std::isalnum(character, locale_); }
  bool isalpha() const { return std::isalpha(character, locale_); }
  bool isblank() const { return std::isblank(character, locale_); }
  bool iscontrol() const;
  bool isdigit() const { return std::isdigit(character, locale_); }
  bool isgraph() const { return std::isgraph(character, locale_); }
  bool islowercase() const { return std::islower(character, locale_); }
  bool isprint() const { return std::isprint(character, locale_); }
  bool ispunctuation() const { return std::ispunct(character, locale_); }
  bool isspace() const { return std::isspace(character, locale_); }
  bool isuppercase() const { return std::isupper(character, locale_); }

  const Rune & operator = (const char c) { character = c; return *this; }
  bool operator == (const char c) const { return character == c; }
  bool operator < (const Rune &) const; 
  operator std::string() const;

  friend RuneFactory;
  friend std::ostream & operator << (std::ostream &, const Rune &);

private:
  static const std::locale locale_;
};

struct RuneFactory {
  Rune make(const wchar_t);
  void reset();
  Color backgroundColor;
  Color foregroundColor;
  bool hasBackgroundColor = false;
  bool hasForegroundColor = false;
  bool isBold = false;
  bool isItalic = false;
};
} // end of rune namespace
