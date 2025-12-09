// Copyright Daniel Morilha 2025

#pragma once

#include <functional>
#include <memory>
#include <string>

#include <pty.h>

#include "poller.h"

struct Screen;

struct Terminal : public Events {
  static std::unique_ptr<Terminal> New(Screen &);

  int childfd() const;

  void pollhup() override;
  bool pollin(const std::optional<TimePoint> &) override;
  void resize(int32_t, int32_t);

  void write(const char * const, const size_t);
  void write(const std::string & s) { write(s.c_str(), s.size()); }

  template <typename ... Args>
  void escape(const char * const format, Args ... args) {
    std::array<char, 1024> escape_sequence{'\0',};
    const int length = snprintf(escape_sequence.data(),
        escape_sequence.size(), format, args...);
    write(escape_sequence.data(), length);
  }

protected:
  Terminal(Screen &);
  const static std::string path;
  Screen & screen_;
  struct winsize winsize_{
    .ws_row = 0,
    .ws_col = 0,
  };
  struct {
    int child = 0;
    int parent = 0;
  } fd_;
  pid_t pid_ = 0;
  std::array<char, 4096> buffer_;
  uint16_t bufferStart_ = 0;
};
