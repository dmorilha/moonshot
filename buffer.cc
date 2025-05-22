#include <iostream>

#include "buffer.h"

void Buffer::clear() {
  container_.clear();
  lines_.clear();
  lines_.push_back(0);
  state_ = INITIAL;
}

void Buffer::push_back(Rune && rune) {
  bool newLine = false;
  if (rune.iscontrol()) {
    switch (rune.character) {
    case '\a': // BELL
      // what should we do?
      return;
      break;

    case '\b': // BACKSPACE
      {
        return;
      }
      break;

    case '\n': // NEW LINE
      newLine = true;
      break;

    default:
      break;
    }
  }

  container_.emplace_back(rune);

  ++lines_.back();
  if (newLine) {
    lines_.push_back(0);
    state_ = INITIAL;
  }
}

#if 0
int main() {
  Buffer buffer;

  buffer.push_back(Rune{L'H'});
  buffer.push_back(Rune{L'e'});
  buffer.push_back(Rune{L'l'});
  buffer.push_back(Rune{L'l'});
  buffer.push_back(Rune{L'o'});

  buffer.push_back(Rune{L'\n'});

  buffer.push_back(Rune{L'W'});
  buffer.push_back(Rune{L'o'});
  buffer.push_back(Rune{L'r'});
  buffer.push_back(Rune{L'l'});
  buffer.push_back(Rune{L'd'});

  buffer.push_back(Rune{L'\n'});

  buffer.push_back(Rune{L'F'});
  buffer.push_back(Rune{L'o'});
  buffer.push_back(Rune{L'r'});
  buffer.push_back(Rune{L'e'});
  buffer.push_back(Rune{L'v'});
  buffer.push_back(Rune{L'e'});
  buffer.push_back(Rune{L'r'});
  buffer.push_back(Rune{L'!'});

  for (int i = 0; i < buffer.lines(); ++i) {
    const auto line = buffer[i];
    for (auto item : line) {
      std::cout << item;
    }
  }

  std::cout << std::endl;
  return 0;
}
#endif
