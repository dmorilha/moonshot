#pragma once

#include <map>

#include "font.h"
#include "opengl.h"
#include "rune.h"

struct Character {
  opengl::Texture texture;
  int16_t left = 0;
  int16_t top = 0;
  uint16_t height = 0;
  uint16_t width = 0;

  Character() = default;
  Character(const Character &) = delete;
  Character & operator = (const Character &) = delete;
};

struct CharacterMap {
  using Map = std::map<rune::Rune, Character>;
  CharacterMap();
  const Character & retrieve(const rune::Rune & rune);
  Font & font() { return font_; }
  Font font_;
  Map map_;
};
