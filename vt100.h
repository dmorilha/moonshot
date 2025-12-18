// Copyright Daniel Morilha 2025

#pragma once

#include <array>

#include "terminal.h"

struct vt100 : public Terminal {
  vt100(Screen &);
protected:
  void handleEscape(const char * const, const int);

private:
  enum class CharacterType {
    nonterminal,
    terminal,
  };

  using EscapeSequence = std::vector<char>;
  bool pollin(const std::optional<TimePoint> &) override;

  CharacterType handleCharacter(const wchar_t);

  void alternative_buffer_on();
  void alternative_buffer_off();

  void handleAPC(const char);
  void handleCSI(const char);
  void handleDCS(const char);
  void handleDecMode(const unsigned int, const bool);
  void handleOSC(const char);
  void handlePM(const char);
  void handleSGR();
  Color handleSGRColor(const std::vector<int> &);
  void handleSGRCommand(const int);

  void reportDeviceStatus(const int32_t);


  EscapeSequence escapeSequence_;

  rune::RuneFactory rune_factory_;

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

  uint16_t bufferIndex_ = 0;
  uint16_t bufferSize_ = 0;
  uint8_t bufferStart_ = 0;
};

