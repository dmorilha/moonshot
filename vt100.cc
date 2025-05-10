#include <iostream>
#include <vector>

#include <cassert>

#include "char.h"
#include "screen.h"
#include "vt100.h"

using Buffer = std::vector<Char>;

vt100::vt100(Screen * const screen) : Terminal(screen) { }

void vt100::pollin() {
  Screen::Buffer output;
  std::array<char, 1025> buffer{'\0'};
  assert(nullptr != screen_);
  ssize_t length = read(fd_.child, buffer.data(), buffer.size() - 1);
  while (0 < length) {
    for (std::size_t i = 0; i < length; ++i) {
      const char c = buffer[i];
      if (std::isprint(c)) {
        std::cout << c << " ";
      } else {
        std::cout << "* ";
      }
      if (CSI == state_) {
        switch (c) {
        case 'H': // move cursor
          state_ = DEFAULT;
          break;
        case 'J': // clear screen
          state_ = DEFAULT;
          screen_->clear();
          break;
        case '?':
          state_ = DEFAULT;
          break;
        }
      } else if (ESCAPE == state_) {
        switch (c) {
        case '[':
          state_ = CSI;
          break;
        default:
          state_ = DEFAULT;
          break;
        }
      } else if (DEFAULT == state_)  {
        switch (c) {
        case '\e':
          state_ = ESCAPE;
          break;
        default:
          output.push_back(Char{ .character = buffer[i], });
          break;
        }
      }
    }
    std::cout << std::endl;
    length = read(fd_.child, buffer.data(), buffer.size() - 1);
  }
  screen_->write(output);
}
