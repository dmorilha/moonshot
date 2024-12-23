#pragma once

#include "freetype.h"

struct Graphics {
  void loadFont(freetype::Face &&);
  void resize(const std::size_t, const std::size_t);
  void draw(const char);

  inline std::size_t columns(void) const { return columns_; }
  inline std::size_t rows(void) const { return rows_; }

// private:
  void calculateGeometries();

  freetype::Face freetypeFace_;
  std::size_t columns_ = 0;
  std::size_t height_ = 0;
  std::size_t padding_ = 1; /*px*/
  std::size_t rows_ = 0;
  std::size_t width_ = 0;

  struct {
    std::size_t height = 0;
    std::size_t width = 0;
  } cell_;
};
