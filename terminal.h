#pragma once

#include <functional>
#include <memory>
#include <string>

#include <pty.h>

#include "poller.h"

struct Screen;

struct Terminal : public Events {
  static std::unique_ptr<Terminal> New(Screen * const);

  int childfd() const;
  void pollhup() override;
  void pollin() override;
  void resize(int32_t width, int32_t height);
  void write(const char * const, const std::size_t);

protected:
  Terminal(Screen * const);
  const static std::string path;
  Screen * const screen_ = nullptr;
  struct winsize winsize_{
    .ws_row = 0,
    .ws_col = 0,
  };
  struct {
    int child = 0;
    int parent = 0;
  } fd_;
  pid_t pid_ = 0;
};
