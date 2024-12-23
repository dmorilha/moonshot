#include <algorithm>
#include <iostream>

#include <cassert>

#include <fcntl.h>
#include <pty.h>
#include <utmp.h>

#include "terminal.h"

const std::string Terminal::path = "/bin/bash";

Terminal::Terminal() : Events(POLLIN) { }

void Terminal::pollin() {
  Buffer buffer{'\0'};
  while (0 < read(fd_.child, buffer.data(), buffer.size() - 1)) {
    if (static_cast<bool>(onRead_)) {
      onRead_(buffer);
    }
  }
}

int Terminal::childfd() const {
  return fd_.child;
}

std::unique_ptr<Terminal> Terminal::New(Terminal::OnRead && onRead) {
  std::unique_ptr<Terminal> instance{new Terminal};

  instance->onRead_ = std::move(onRead);

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
    unsetenv("TERM");
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

void Terminal::write(const Terminal::Buffer & buffer) {
  assert(0 < fd_.child);
  const ssize_t result = ::write(fd_.child, buffer.data(), std::find(buffer.begin(), buffer.end(), '\0') - buffer.begin());
  assert(0 <= result);
}
