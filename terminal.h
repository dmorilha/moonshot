#pragma once

#include <functional>
#include <memory>
#include <string>

#include "poller.h"

struct Screen;

/* add support to terminal state machine emulation */
struct Terminal : public Events {
  static std::unique_ptr<Terminal> New(Screen * const);

  int childfd() const;
  void pollhup() override;
  void pollin() override;
  void write(const char * const, const std::size_t);

protected:
  Terminal(Screen * const);
  const static std::string path;
  Screen * const screen_ = nullptr;
  struct {
    int child = 0;
    int parent = 0;
  } fd_;
  pid_t pid_ = 0;
};
