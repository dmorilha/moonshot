#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <vector>

#include <cassert>
#include <cwchar>

#include <fcntl.h>
#include <pty.h>
#include <utmp.h>

#include <unistd.h>

#include "buffer.h"
#include "screen.h"
#include "terminal.h"

const std::string Terminal::path = "/bin/bash";

Terminal::Terminal(Screen * const screen) : Events(POLLIN | POLLHUP), screen_(screen) { }

void Terminal::pollin() {
  assert(nullptr != screen_);

  ssize_t length = 0;
  {
    const ssize_t result = read(fd_.child, buffer_.data() + bufferStart_, buffer_.size() - bufferStart_);
    if (0 > result) {
      if (EAGAIN == errno) {
        return;
      } else {
        assert(!"ERROR");
      }
    }
    length = bufferStart_ + result;
  }
  while (bufferStart_ < length) {
    uint16_t iterator = 0;
    while (length > iterator) {
      wchar_t character;
      const size_t size = length - iterator;
      const ssize_t bytes = mbrtowc(&character, &buffer_[iterator], size, nullptr);
      assert(4 >= bytes);
      assert(bytes <= size);
      #if DEBUG
      std::array<char, 5> display{'\0'};
      strncpy(display.data(), static_cast<const char *>(&buffer_[iterator]), bytes);
      std::cout << __func__ << ": " << iterator << ", " << bufferStart_ << ", " << length << ", " << size << " " << display.data() << std::endl;
      #endif //DEBUG
      switch (bytes) {
      case -2: /* INCOMPLETE */
        return;
        if (0 < iterator) {
          strncpy(&buffer_[0], &buffer_[iterator], size);
        }
        bufferStart_ = iterator = size;
        assert(4 > bufferStart_);
        break;
      case -1: /* INVALID */
        std::cout << "INVALID " << iterator << ", " << bufferStart_ << ", " << length << ", " << size << ", " << bytes << std::endl;
        assert(!"INVALID");
        break;
      case 0: /* ODD */
        bufferStart_ = 0;
        break;
      default:
        screen_->buffer().push_back(Rune{ .character = character });
        iterator += bytes;
        bufferStart_ = 0;
        break;
      }
    }

    {
      const ssize_t result = read(fd_.child, buffer_.data() + bufferStart_, buffer_.size() - bufferStart_);
      if (0 > result) {
        if (EAGAIN == errno) {
          break;
        } else {
          assert(!"ERROR");
        }
      }
      length = bufferStart_ + result;
    }

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
  std::unique_ptr<Terminal> instance = nullptr;
  assert(nullptr != screen);

  const char * const TERM = getenv("TERM");
  std::string terminalType;
  if (nullptr != TERM && 0 < strlen(TERM)) {
    terminalType = getenv("TERM");
  }

  std::transform(terminalType.begin(), terminalType.end(), terminalType.begin(),
    std::bind(std::tolower<std::string::value_type>, std::placeholders::_1, std::locale::classic()));

  unsetenv("TERM");
  instance = std::unique_ptr<Terminal>(new Terminal(screen));

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

void Terminal::write(const char * const key, const size_t length) {
  assert(0 < fd_.child);
  const ssize_t result = ::write(fd_.child, key, length);
  assert(0 <= result);
}

void Terminal::resize(int32_t width, int32_t height) {
  assert(nullptr != screen_);
  winsize_.ws_col = screen_->getColumns();
  winsize_.ws_row = screen_->getLines();
  assert(0 < fd_.child);
  const int result = ioctl(fd_.child, TIOCSWINSZ, &winsize_);
  assert(0 == result);
}
