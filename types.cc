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
