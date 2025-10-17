#pragma once

#include <array>
#include <map>

#include "terminal.h"

struct vt100 : public Terminal {
  vt100(Screen * const);

  const std::map<int, Color> Colors {
    {30, color::black},
    {31, color::red},
    {32, color::green},
    {33, color::yellow},
    {34, color::blue},
    {35, color::magenta},
    {36, color::cyan},
    {37, color::white},
    {40, color::black},
    {41, color::red},
    {42, color::green},
    {43, color::yellow},
    {44, color::blue},
    {45, color::magenta},
    {46, color::cyan},
    {47, color::white},
  };

private:
  using EscapeSequence = std::vector<char>;
  void pollin() override;

  void handleCharacter(const char);

  void handleAPC(const char);
  void handleCSI(const char);
  void handleDCS(const char);
  void handleDecMode(const unsigned int, const bool);
  void handleOSC(const char);
  void handlePM(const char);
  void handleSGR();
  void handleSGRCommand(const int);

  void reportDeviceStatus(const int32_t);

  EscapeSequence escapeSequence_;

  rune::RuneFactory runeFactory_;

  enum {
    LITERAL,

    ESCAPE,

    APC,
    CSI,
    DCS,
    DEC,
    OSC,
    PM,

    DEC_SPECIAL,

    CHARSET_G0,
    CHARSET_G1,
    CHARSET_G2,
    CHARSET_G3,

    OTHER_ESC_CTL_SEQ,

    TITLE,

    UPPER_BOUND,
  } state_ = LITERAL;
};
