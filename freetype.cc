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

Face Library::load(const std::string & filename, const std::size_t size, const std::size_t dpi) {
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

  face.descender_ = face.face_->size->metrics.descender / 64;
  face.glyphWidth_ = face.face_->glyph->advance.x / 64;
  face.lineHeight_ = face.face_->size->metrics.height / 64;

#if 0
  std::cout
    << "face.face_->size->metrics.ascender "
    << face.face_->size->metrics.ascender
    << "\nface.face_->size->metrics.descender "
    << face.face_->size->metrics.descender
    << "\nface.face_->size->metrics.height "
    << face.face_->size->metrics.height
    << "\nface.face_->size->metrics.max_advance "
    << face.face_->size->metrics.max_advance
    << "\nface.face_->size->metrics.x_ppem "
    << face.face_->size->metrics.x_ppem
    << "\nface.face_->size->metrics.x_scale "
    << face.face_->size->metrics.x_scale
    << "\nface.face_->size->metrics.y_ppem "
    << face.face_->size->metrics.y_ppem
    << "\nface.face_->size->metrics.y_scale "
    << face.face_->size->metrics.y_scale
    << "\nface.face_->glyph->advance.x "
    << face.face_->glyph->advance.x
    << "\nface.face_->glyph->advance.y "
    << face.face_->glyph->advance.y
    << std::endl;
#endif


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
  descender_(other.descender_),
  lineHeight_(other.lineHeight_),
  glyphWidth_(other.glyphWidth_) { }

Face & Face::operator = (Face && other) {
  std::swap(face_, other.face_);
  descender_ = other.descender_;
  lineHeight_ = other.lineHeight_;
  glyphWidth_ = other.glyphWidth_;
  return *this;
}

Glyph Face::glyph(const char codepoint) const {
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

  #if 0
  std::cout << "Is face_->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ? " << (face_->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_MONO ? "yes" : "no") << std::endl;
  #endif

  assert(nullptr != face_->glyph);

  result.height = face_->glyph->bitmap.rows;
  result.left = face_->glyph->bitmap_left;
  result.pixels = face_->glyph->bitmap.buffer;
  result.slot = face_->glyph;
  result.top = face_->glyph->bitmap_top;
  result.width = face_->glyph->bitmap.width;

  return result;
}
} // end of freetype namespace

#if 0
int main() {
  freetype::Library l;
  auto face = l.load("/usr/share/fonts/liberation-fonts/LiberationMono-Regular.ttf", 64);
  freetype::Glyph glyph = face.glyph('0');
  return 0;
}
#endif
