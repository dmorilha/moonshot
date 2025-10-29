#pragma once

#include "font.h"
#include "rune.h"

struct CharacterMap {
  CharacterMap();
  void retrieve(rune::Rune & rune);
  Font & font() { return font_; }
  Font font_;
};
