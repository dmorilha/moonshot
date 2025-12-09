// Copyright Daniel Morilha 2025

#include "types.h"

Rectangle_Y::operator Rectangle () const {
  if (std::numeric_limits<int32_t>::max() < y) {
    std::cerr << "narrowing " << y << " to an int32_t" << std::endl;
  }
  return Rectangle{
    .x = x,
    .y = static_cast<int32_t>(y),
    .width = width,
    .height = height,
  };
}

std::ostream & operator << (std::ostream & o, const Color & c) {
  o << "(R:" << c.red << ", G:" << c.green << ", B:" << c.blue << ", A:" << c.alpha << ")";
  return o;
}

std::ostream & operator << (std::ostream & o, const Rectangle & r) {
  o << "(" << r.x << "," << r.y << "; " << r.width << "," << r.height << ")";
  return o;
}

std::ostream & operator << (std::ostream & o, const Rectangle_Y & r) {
  o << "(" << r.x << "," << r.y << "; " << r.width << "," << r.height << ")";
  return o;
}

bool Rectangle::contains(const Rectangle & o) const {
  return x <= o.x && y <= o.y
    && x + width >= o.x + o.width
    && y + height >= o.y + o.height;
}

bool Rectangle::overlaps(const Rectangle & o) const {
  // there is more to it
  return (o.x <= x && x <= o.x1() && o.y <= y && y <= o.y1()) ||
    (x <= o.x && o.x <= x1() && y <= o.y && o.y <= y1());
}

bool Rectangle::incorporate(const Rectangle & o) {
  if (o.y == y && o.height == height) {
    const int32_t new_x = std::min(x, o.x);
    width = std::max(x1(), o.x1()) - new_x;
    x = new_x;
    return true;
  } else if (o.x == x && o.width == width) {
    const int32_t new_y = std::min(y, o.y);
    height = std::max(y1(), o.y1()) - new_y;
    y = new_y;
    return true;
  }
  return false;
}

bool Rectangle::operator < (const Rectangle & o) const {
  if (y > o.y) {
    return true;
  } else if (y == o.y) {
    if (x < o.x) {
      return true;
    } else if (x == o.x) {
      if (height < o.height) {
        return true;
      } else if (height == o.height && width < o.width) {
        return true;
      }
    }
  }
  return false;
}
