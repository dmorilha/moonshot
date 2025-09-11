#include <algorithm>
#include <iostream>
#include <vector>

#include "rune.h"
#include "freetype.h"
#include "poller.h"
#include "screen.h"
#include "terminal.h"
#include "wayland.h"

template<class PAINT>
struct WaylandPoller : public Events {
  WaylandPoller(wayland::Connection & c, PAINT && p) : Events(POLLIN), connection_(c), paint(p) { }
  void pollin() override;
private:
  PAINT paint;
  wayland::Connection & connection_;
};

template<class F>
void WaylandPoller<F>::pollin() {
  paint();
  connection_.roundtrip();
}

int main(int argc, char ** argv) {
  wayland::Connection connection;
  connection.connect();
  connection.capabilities();

  auto font = Font::New({
    .bold = "/usr/share/fonts/liberation-fonts/LiberationMono-Bold.ttf",
    .boldItalic = "/usr/share/fonts/liberation-fonts/LiberationMono-BoldItalic.ttf",
    .italic = "/usr/share/fonts/liberation-fonts/LiberationMono-Italic.ttf",
    .regular = "/usr/share/fonts/liberation-fonts/LiberationMono-Regular.ttf",
    .size = 15,
  });

  Screen screen{Screen::New(connection, std::move(font))};

  connection.roundtrip();

  Poller poller;

  auto t = Terminal::New(&screen);
  const int fd = t->childfd();
  Terminal & terminal = poller.add(fd, std::move(t));

  poller.add(connection.fd(), std::unique_ptr<Events>(new WaylandPoller(connection, [&]() {
    screen.repaint();
  })));

  connection.onKeyPress = [&](const uint32_t key, const char * const utf8, const size_t bytes, const uint32_t modifiers) {
    // example as how to certain key sequences should be handled outside the shell.
    if (0 != (modifiers & 0x4 /* crtl key */)) {
      switch (key) {
      case XKB_KEY_plus:
        screen.increaseFontSize();
        return;
      case XKB_KEY_minus:
        screen.decreaseFontSize();
        return;
      default:
        break;
      }
    }
    screen.resetScroll(); // isn't what scroll lock is for ?
    terminal.write(utf8, bytes);
  };

  connection.onPointerAxis = [&](uint32_t axis, int32_t value) {
    // how to handle keyboard modifiers to change different axis on a mouse
    enum {
      Y = 0,
      X = 1,
    };
    switch (axis) {
    case Y:
      screen.changeScrollY(value);
      break;
    case X:
      break;
    }
  };

  struct {
    bool leftPressed = false;
    int32_t x = 0;
    int32_t y = 0;
  } pointer;

  connection.onPointerMotion = [&](int32_t x, int32_t y) {
    pointer.x = 0 > x ? 0 : x;
    pointer.y = 0 > y ? 0 : y;
    if (pointer.leftPressed) {
      screen.drag(pointer.x, pointer.y);
    }
  };

  connection.onPointerButton = [&](uint32_t button, uint32_t state) {
    enum {
      RELEASE = 0,
      CLICK = 1,
      LEFT = 272,
      RIGHT = 273,
    };

    if (LEFT == button) {
      pointer.leftPressed = CLICK == state;
    }

    if (CLICK == state) {
      switch (button) {
      case LEFT:
        screen.startSelection(pointer.x, pointer.y);
        break;
      default:
        break;
      }
    } else if (RELEASE == state) {
      switch (button) {
      case LEFT:
        screen.endSelection();
        break;
      default:
        break;
      }
    }
  };

  screen.onResize = [&](int32_t width, int32_t height) {
    terminal.resize(width, height);
  };

  poller.on();
  poller.poll();

  return 0;
}
