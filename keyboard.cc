#include <string>

#include <xkbcommon/xkbcommon-keysyms.h>
#include <xkbcommon/xkbcommon.h>

#include "keyboard.h"

void Keyboard::on_key_press(const uint32_t key, const char * const utf8, const size_t bytes, const uint32_t modifiers) {
    
#if 0
  // example as how to certain key sequences should be handled outside the shell.
  if (0 != (modifiers & 0x4 /* crtl key */)) {
    switch (key) {
      case XKB_KEY_plus:
        // screen.increaseFontSize();
        return;
      case XKB_KEY_minus:
        // screen.decreaseFontSize();
        return;
      default:
        break;
    }
  }
#endif

  switch (key) {
  case XKB_KEY_Down: 
    terminal_.write(std::string{"\e[B"});
    return;
    break;
  case XKB_KEY_Left:
    terminal_.write(std::string{"\e[D"});
    return;
    break;
  case XKB_KEY_Right:
    terminal_.write(std::string{"\e[C"});
    return;
    break;
  case XKB_KEY_Up:
    terminal_.write(std::string{"\e[A"});
    return;
    break;
  }

  terminal_.write(utf8, bytes);
}

