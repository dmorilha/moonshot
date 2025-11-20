#pragma once

#include "screen.h"
#include "terminal.h"

struct Keyboard {
  Keyboard(Screen & screen, Terminal & terminal): screen_(screen), terminal_(terminal) { }
  auto on_key_press(const uint32_t, const char * const, const size_t, const uint32_t) -> void;
private:
  Screen & screen_;
  Terminal & terminal_;
};
