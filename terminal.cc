#include <algorithm>
#include <array>
#include <functional>
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

  const char * const TERM = getenv("TERM");
  std::string terminalType;
  if (nullptr != TERM && 0 < strlen(TERM)) {
    terminalType = getenv("TERM");
  }

  std::transform(terminalType.begin(), terminalType.end(), terminalType.begin(),
    std::bind(std::tolower<std::string::value_type>, std::placeholders::_1, std::locale::classic()));

  if ("vt100" == terminalType) {
    instance = std::unique_ptr<Terminal>(new vt100(screen));
  } else {
    unsetenv("TERM");
    instance = std::unique_ptr<Terminal>(new Terminal(screen));
  }

  instance->winsize_.ws_col = screen->getColumns();
  instance->winsize_.ws_row = screen->getLines();
  openpty(&instance->fd_.child, &instance->fd_.parent, nullptr, nullptr, &instance->winsize_);

  instance->pid_ = fork();
  if (0 == instance->pid_) { /* CHILD */
    close(instance->fd_.child);
    login_tty(instance->fd_.parent);
    unsetenv("COLORTERM");
    unsetenv("TERMCAP");
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

void Terminal::resize(int32_t width, int32_t height) {
  assert(nullptr != screen_);
  winsize_.ws_col = screen_->getColumns();
  winsize_.ws_row = screen_->getLines();
  #if 0
  winsize_.ws_xpixel = 1;
  winsize_.ws_ypixel = 1;
  #endif
  assert(0 < fd_.child);
  const int result = ioctl(fd_.child, TIOCSWINSZ, &winsize_);
  assert(0 <= result);
}
