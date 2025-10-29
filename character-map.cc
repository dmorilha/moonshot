#include "character-map.h"

CharacterMap::CharacterMap() :
  font_(Font::New({
        .bold = "/usr/share/fonts/liberation-fonts/LiberationMono-Bold.ttf",
        .boldItalic = "/usr/share/fonts/liberation-fonts/LiberationMono-BoldItalic.ttf",
        .italic = "/usr/share/fonts/liberation-fonts/LiberationMono-Italic.ttf",
        .regular = "/usr/share/fonts/liberation-fonts/LiberationMono-Regular.ttf",
        .size = 15,
        })) { }

const Character & CharacterMap::retrieve(const rune::Rune & rune) {
  Map::const_iterator iterator = map_.find(rune);
  const bool found = map_.cend() != iterator;
  if ( ! found) {
    Character & character = map_[rune];
    freetype::Glyph glyph;
    switch (rune.style) {
    case rune::Style::REGULAR:
      glyph = font_.regular().glyph(rune.character);
      break;
    case rune::Style::BOLD:
      glyph = font_.bold().glyph(rune.character);
      break;
    case rune::Style::ITALIC:
      glyph = font_.italic().glyph(rune.character);
      break;
    case rune::Style::BOLD_AND_ITALIC:
      glyph = font_.boldItalic().glyph(rune.character);
      break;
    }

    // dimensions
    character.height = glyph.height;
    character.left = glyph.left;
    character.top = glyph.top;
    character.width = glyph.width;

    // texture
    glBindTexture(GL_TEXTURE_2D, character.texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, glyph.width, glyph.height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, glyph.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return character;
  }
  return iterator->second;
}
