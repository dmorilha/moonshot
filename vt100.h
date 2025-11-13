#pragma once

#include <array>
#include <map>

#include "terminal.h"

struct vt100 : public Terminal {
  vt100(Screen * const);

  const std::map<int, Color> Colors {
    {30, colors::black},
    {31, colors::red},
    {32, colors::green},
    {33, colors::yellow},
    {34, colors::blue},
    {35, colors::magenta},
    {36, colors::cyan},
    {37, colors::white},
    {40, colors::black},
    {41, colors::red},
    {42, colors::green},
    {43, colors::yellow},
    {44, colors::blue},
    {45, colors::magenta},
    {46, colors::cyan},
    {47, colors::white},
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
  Color handleSGRColor(const std::vector<const char *> &);
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
