#include <iostream>

#include <cassert>

#include "freetype.h"

namespace freetype {

Library::~Library() {
  assert(nullptr != library_);
  FT_Done_FreeType(library_);
  library_ = nullptr;
}

Library::Library() {
  {
    const FT_Error error = FT_Init_FreeType(&library_);
    if (0 != error) {
      std::cerr << "Failed to initialize freetype " << FT_Error_String(error) << std::endl;
    }
  }

  //TODO: check what it does?
  {
    const FT_Error error = FT_Library_SetLcdFilter(library_, FT_LCD_FILTER_DEFAULT);
    if (0 != error) {
      std::cerr << "Freetype has no clear type support " << FT_Error_String(error) << std::endl;
    }
  }
}

Face Library::load(const std::string & filename, const uint16_t size, const uint16_t dpi) {
  assert(nullptr != library_);
  Face face;

  {
    const FT_Error error = FT_New_Face(library_, filename.c_str(), 0, &face.face_);
    if (0 != error) {
      std::cerr << "Failed to load font file " << filename << " " << FT_Error_String(error) << std::endl;
    }
  }

  const FT_F26Dot6 internalSize = std::max<std::size_t>(size, 1) * 64;
  const FT_UInt internalResolution = dpi;

  {
    const FT_Error error = FT_Set_Char_Size(face.face_, internalSize, internalSize, internalResolution, internalResolution);

    if (0 != error) {
      std::cerr << "Failed to set font size to " << size << " " << FT_Error_String(error) << std::endl;
    }
  }

  {
    const FT_Error error = FT_Load_Glyph(face.face_, '(', FT_LOAD_TARGET_NORMAL);
    if (0 != error) {
      std::cerr << "Failed to load glyph " << FT_Error_String(error) << std::endl;
    }
  }

  face.ascender_ = face.face_->size->metrics.ascender / 64;
  face.descender_ = face.face_->size->metrics.descender / 64;
  face.glyphWidth_ = face.face_->glyph->advance.x / 64;
  face.lineHeight_ = face.face_->size->metrics.height / 64;

  return face;
}

Face::~Face() {
  if (nullptr != face_) {
    FT_Done_Face(face_);
    face_ = nullptr;
  }
}

Face::Face(Face && other) :
  face_(std::move(other.face_)),
  ascender_(other.ascender_),
  descender_(other.descender_),
  lineHeight_(other.lineHeight_),
  glyphWidth_(other.glyphWidth_) { }

Face & Face::operator = (Face && other) {
  std::swap(face_, other.face_);
  ascender_ = other.ascender_;
  descender_ = other.descender_;
  lineHeight_ = other.lineHeight_;
  glyphWidth_ = other.glyphWidth_;
  return *this;
}

Glyph Face::glyph(const wchar_t codepoint) const {
  assert(nullptr != face_);
  Glyph result;

  const FT_UInt index = FT_Get_Char_Index(face_, codepoint);

  {
    const FT_Error error = FT_Load_Glyph(face_, index, FT_LOAD_TARGET_NORMAL);
    if (0 != error) {
      std::cerr << "Glyph load error " << FT_Error_String(error) << std::endl;
    }
  }

  {
    const FT_Error error = FT_Render_Glyph(face_->glyph, FT_RENDER_MODE_NORMAL);
    if (0 != error) {
      std::cerr << "Glyph render error " << FT_Error_String(error) << std::endl;
    }
  }

  assert(nullptr != face_->glyph);

  result.height = face_->glyph->bitmap.rows;
  result.left = face_->glyph->bitmap_left;
  result.pixels = face_->glyph->bitmap.buffer;
  result.slot = face_->glyph;
  result.top = face_->glyph->bitmap_top;
  result.width = face_->glyph->bitmap.width;

  return result;
}

std::ostream & operator << (std::ostream & o, const Face & face) {
    o << "size.metrics.ascender "
      << face.face_->size->metrics.ascender << std::endl
      << "size.metrics.descender "
      << face.face_->size->metrics.descender << std::endl
      << "size.metrics.height "
      << face.face_->size->metrics.height << std::endl
      << "size.metrics.max_advance "
      << face.face_->size->metrics.max_advance << std::endl
      << "size.metrics.x_ppem "
      << face.face_->size->metrics.x_ppem << std::endl
      << "size.metrics.x_scale "
      << face.face_->size->metrics.x_scale << std::endl
      << "size.metrics.y_ppem "
      << face.face_->size->metrics.y_ppem << std::endl
      << "size.metrics.y_scale "
      << face.face_->size->metrics.y_scale << std::endl
      << "glyph.advance.x "
      << face.face_->glyph->advance.x << std::endl
      << "glyph.advance.y "
      << face.face_->glyph->advance.y << std::endl;
    return o;
}
} // end of freetype namespace
