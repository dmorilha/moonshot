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

  if (hasForegroundColor) {
    rune.hasForegroundColor = true;
    rune.foregroundColor = foregroundColor;
  }

  return rune;
}

void RuneFactory::reset() {
  hasBackgroundColor = false;
  hasForegroundColor = false;
  isBold = false;
  isItalic = false;
}

} // end of rune namespace
