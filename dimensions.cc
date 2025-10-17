#include <iostream>

#include "dimensions.h"

std::ostream & operator << (std::ostream & o, const Dimensions & d) {
  o << "(column " << d.column
    << ", columns " << d.columns()
    << ", line " << d.line
    << ", lines " << d.lines()
    << ", totalLines " << d.totalLines << ")"
    << " (cursorLeft " << d.cursorLeft
    << ", cursorTop " << d.cursorTop << ")"
    << " (glyphDescender " << d.glyphDescender
    << ", glyphWidth " << d.glyphWidth
    << ", lineHeight " << d.lineHeight << ")"
    << " (scaleHeight " << d.scaleHeight()
    << ", scaleWidth " << d.scaleWidth() << ")"
    << " (screenTop " << d.screenTop
    << ", scrollY " << d.scrollY
    << ", surfaceHeight " << d.surfaceHeight
    << ", surfaceWidth " << d.surfaceWidth << ")"
    << " (bottomPadding " << d.bottomPadding
    << ", leftPadding " << d.leftPadding << ")";
  return o;
}

