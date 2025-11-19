#include <ostream>
#include <array>

#include <cstring>

#include "rune.h"

namespace rune {
const std::locale Rune::locale_("");

Rune::operator std::string() const {
  std::array<char, 5> buffer{'\0'};
  memcpy(buffer.data(), static_cast<const void *>(&character), 4);
  return std::string(buffer.data());
}

std::ostream & operator << (std::ostream & o, const Rune & rune) {
  o << static_cast<std::string>(rune);
  return o;
}

bool Rune::iscontrol() const { return std::iscntrl(character, locale_); }

bool Rune::operator < (const Rune & o) const {
  return character < o.character || style < o.style;
}

Rune RuneFactory::make(const wchar_t c) {
  Rune rune(c);
  if (isBold && isItalic) {
    rune.style = Style::BOLD_AND_ITALIC;
  } else if (isBold) {
    rune.style = Style::BOLD;
  } else if (isItalic) {
    rune.style = Style::ITALIC;
  } else {
    rune.style = Style::REGULAR;
  }

  rune.foregroundColor = foregroundColor;
  rune.backgroundColor = backgroundColor;
  rune.blink = blink;

  return rune;
}

void RuneFactory::reset() {
  operator = (RuneFactory());
}

} // end of rune namespace
