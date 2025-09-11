#include <iostream>

#include <cassert>

#include "dimensions.h"

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "bottomPadding " << d.bottomPadding
    << ", column " << d.column
    << ", columns " << d.columns()
    << ", cursorLeft " << d.cursorLeft
    << ", cursorTop " << d.cursorTop
    << ", glyphAscender " << d.glyphAscender
    << ", glyphDescender " << d.glyphDescender
    << ", glyphWidth " << d.glyphWidth
    << ", leftPadding " << d.leftPadding
    << ", line " << d.line
    << ", lineHeight " << d.lineHeight
    << ", lines " << d.lines()
    << ", scaleHeight " << d.scaleHeight()
    << ", scaleWidth " << d.scaleWidth()
    << ", scrollX " << d.scrollX
    << ", scrollY " << d.scrollY
    << ", screenTop " << d.screenTop
    << ", surfaceHeight " << d.surfaceHeight
    << ", surfaceWidth " << d.surfaceWidth;
  return o;
}

