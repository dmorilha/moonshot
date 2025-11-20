#include <chrono>
#include <memory>

#include "freetype.h"
#include "keyboard.h"
#include "poller.h"
#include "screen.h"
#include "terminal.h"
#include "wayland.h"

template<class PAINT>
struct WaylandPoller : public Events {
  WaylandPoller(wayland::Connection & c, PAINT && p) : Events(POLLIN), connection_(c), paint(p) { }
  void pollin() override;
  void timeout() override;
private:
  constexpr auto millis() -> uint16_t {
    const auto now = std::chrono::steady_clock::now();
    const uint16_t millis = (now.time_since_epoch().count() / 1000000) % 1000;
    return millis;
  }

  PAINT paint;
  bool alternative_ = false;
  wayland::Connection & connection_;
};

template<class PAINT>
void WaylandPoller<PAINT>::pollin() {
  const bool flip = 500 <= millis();
  const bool force = alternative_ ^ flip;
  alternative_ = flip;
  paint(force, alternative_);
  connection_.roundtrip();
}

template<class PAINT>
void WaylandPoller<PAINT>::timeout() {
  pollin();
}

int main(int argc, char ** argv) {
  using std::chrono_literals::operator""ms;

  wayland::Connection connection;
  connection.connect();
  connection.capabilities();

  Screen screen{Screen::New(connection)};

  connection.roundtrip();

  Poller poller(/* timeout for 100 fps */ 10ms);

  auto t = Terminal::New(screen);
  const int fd = t->childfd();

  Terminal & terminal = poller.add(fd, std::move(t));

  poller.add(connection.fd(), std::unique_ptr<Events>(new WaylandPoller(connection, [&](const bool force, const bool alt) {
    screen.repaint(force, alt);
  })));

  {
    Keyboard keyboard(screen, terminal);
    connection.onKeyPress = std::bind_front(&Keyboard::on_key_press, keyboard);
  }

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
