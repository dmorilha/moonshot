#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

#include <cassert>

#include <fcntl.h>
#include <pty.h>
#include <utmp.h>

#include <unistd.h>

#include "buffer.h"
#include "screen.h"
#include "terminal.h"
#include "vt100.h"

const std::string Terminal::path = "/bin/bash";

Terminal::Terminal(Screen * const screen) : Events(POLLIN | POLLHUP), screen_(screen) { }

void Terminal::pollin() {
  std::array<char, 1025> buffer{'\0'};
  assert(nullptr != screen_);
  ssize_t length = read(fd_.child, buffer.data(), buffer.size() - 1);
  while (0 < length) {
    for (std::size_t i = 0; i < length; ++i) {
      screen_->buffer().push_back(Char{ .character = buffer[i], });
    }
    length = read(fd_.child, buffer.data(), buffer.size() - 1);
  }
  screen_->write();
}

void Terminal::pollhup() {
  exit(0);
}

int Terminal::childfd() const {
  return fd_.child;
}

std::unique_ptr<Terminal> Terminal::New(Screen * const screen) {
  std::unique_ptr<Terminal> instance = nullptr; //{new Terminal};
  assert(nullptr != screen);

  const std::string terminalType = getenv("TERM");

  if ("vt100" == terminalType) {
    instance = std::unique_ptr<Terminal>(new vt100(screen));
  } else {
    unsetenv("TERM");
    instance = std::unique_ptr<Terminal>(new Terminal(screen));
  }

  const struct winsize winsize{ .ws_row = 38, .ws_col = 136, };

  openpty(&instance->fd_.child, &instance->fd_.parent, nullptr, nullptr, &winsize);
  instance->pid_ = fork();
  if (0 == instance->pid_) { /* CHILD */
    close(instance->fd_.child);
    login_tty(instance->fd_.parent);
    unsetenv("COLUMNS");
    unsetenv("LINES");
    unsetenv("TERMCAP");
    unsetenv("COLORTERM");
    const int returnValue = execvp(Terminal::path.c_str(), nullptr);
    if (0 != returnValue) {
      assert(!"FATAL ERROR");
    }
  } else if (0 > instance->pid_) {
    std::cerr << "failed to fork process" << std::endl;
  }
  close(instance->fd_.parent);
  fcntl(instance->fd_.child, F_SETFL, fcntl(instance->fd_.child, F_GETFL) | O_NONBLOCK);
  return instance;
}

void Terminal::write(const char * const key, std::size_t length) {
  assert(0 < fd_.child);
  const ssize_t result = ::write(fd_.child, key, length);
  assert(0 <= result);
}
