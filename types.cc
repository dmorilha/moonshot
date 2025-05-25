#include "types.h"

std::ostream & operator << (std::ostream & o, const Rectangle & r) {
  o << "(" << r.x << "," << r.y << ") (" << r.width << "," << r.height << ")";
  return o;
}
