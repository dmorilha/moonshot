#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>

#include "poller.h"

/* add support to terminal state machine emulation */
/* each type of terminal inherits from this Basic Terminal */
struct Terminal : public Events {
  using Buffer = std::array<char, 1025>;
  using OnRead = std::function<void (const Buffer &)>;

  static std::unique_ptr<Terminal> New(OnRead &&);

  int childfd() const;
  void pollhup() override;
  void pollin() override;
  void write(const Buffer &);
  void write(const char * const, const std::size_t);

private:
  const static std::string path;

  Terminal();

  OnRead onRead_;
  struct {
    int child = 0;
    int parent = 0;
  } fd_;
  pid_t pid_ = 0;
};
