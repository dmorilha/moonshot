#pragma once

#include <string>

#include <cstdint>

#include <freetype/freetype.h>
#include <freetype/ftlcdfil.h>

namespace freetype {

struct Glyph {
  FT_GlyphSlotRec * slot = nullptr;
  uint16_t height = 0;
  int16_t left = 0;
  int16_t top = 0;
  uint16_t width = 0;
  void * pixels = nullptr;
};

struct Library;

struct Face {
  ~Face();

  Face() = default;
  Face(const Face &) = delete;
  Face(Face && other);
  Face & operator = (const Face &) = delete;
  Face & operator = (Face && other);

  operator bool () const { return nullptr != face_; }

  uint16_t ascender() const {
    return ascender_;
  }

  int16_t descender() const {
    return descender_;
  }

  uint16_t glyphWidth() const {
    return glyphWidth_;
  }

  uint16_t lineHeight() const {
    return lineHeight_;
  }

  Glyph glyph(const wchar_t) const;

  friend std::ostream & operator << (std::ostream &, const Face &);
  friend class Library;

private:
  FT_Face face_ = nullptr;
  uint16_t ascender_ = 0;
  int16_t descender_ = 0;
  uint16_t lineHeight_ = 0;
  uint16_t glyphWidth_ = 0;
};

//TODO: this is most likely a singleton
struct Library {
  ~Library();
  Library();

  Face load(const std::string &, const uint16_t, const uint16_t dpi = 96);

private:
  FT_Library library_ = nullptr;
};

} // end of freetype namespace
