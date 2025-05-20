#pragma once

#include <string>

#include "freetype.h"

struct Font {
  struct Paths {
    std::string bold;
    std::string boldItalic;
    std::string italic;
    std::string regular;
    uint8_t size;
  };

  static Font New(Paths &&);

  Font(const Font &) = delete;
  Font & operator = (const Font &) = delete;
  Font(Font &&) = default;
  Font & operator = (Font &&) = delete;

  freetype::Face & bold();
  freetype::Face & boldItalic();
  freetype::Face & italic();
  freetype::Face & regular();

private:
  Font(Paths && paths) : paths_(std::move(paths)) { }

  Paths paths_;
  freetype::Library freetype_{};

  freetype::Face boldItalic_;
  freetype::Face bold_;
  freetype::Face italic_;
  freetype::Face regular_;
};
