#include <algorithm>
#include <iostream>
#include <vector>

#include "char.h"
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
  Screen screen{Screen::New(connection)};

  int faceSize =  8;
  if (1 < argc) {
    faceSize = std::atoi(argv[1]);
  }

  screen.loadFace("/usr/share/fonts/liberation-fonts/LiberationMono-Regular.ttf", faceSize);

  connection.roundtrip();

  Poller poller;

  auto t = Terminal::New(&screen);
  const int fd = t->childfd();
  Terminal & terminal = poller.add(fd, std::move(t));

  poller.add(connection.fd(), std::unique_ptr<Events>(new WaylandPoller(connection, [&]() {
    screen.repaint();
  })));

  connection.onKeyPress = [&](const char * const key) {
    terminal.write(key, strlen(key));
  };

  connection.onPointerAxisChange = [&](uint32_t axis, int32_t value) {
    switch (axis) {
    case 0:
      screen.changeScrollY(value);
      break;
    case 1:
      screen.changeScrollX(value);
      break;
    }
  };

  screen.onResize = [&](int32_t width, int32_t height) {
    terminal.resize(width, height);
  };

  poller.on();
  poller.poll();

  return 0;
}
