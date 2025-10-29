#include "character-map.h"

CharacterMap::CharacterMap() :
  font_(Font::New({
        .bold = "/usr/share/fonts/liberation-fonts/LiberationMono-Bold.ttf",
        .boldItalic = "/usr/share/fonts/liberation-fonts/LiberationMono-BoldItalic.ttf",
        .italic = "/usr/share/fonts/liberation-fonts/LiberationMono-Italic.ttf",
        .regular = "/usr/share/fonts/liberation-fonts/LiberationMono-Regular.ttf",
        .size = 15,
        })) { }

void CharacterMap::retrieve(rune::Rune & rune) {
}
