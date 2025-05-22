#include <ostream>
#include <array>

#include <cstring>

#include "rune.h"

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
