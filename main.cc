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
  bool repaint = true;

  wayland::Connection connection;
  connection.connect();
  connection.capabilities();
  Screen screen{Screen::New(connection)};

  int faceSize =  12;
  if (1 < argc) {
    faceSize = std::atoi(argv[1]);
  }

  screen.loadFace("/usr/share/fonts/liberation-fonts/LiberationMono-Regular.ttf", faceSize);

  // opengl rendering
#if 0
  screen.makeCurrent();
  screen.swapBuffers();
#endif

  connection.roundtrip();

  Poller poller;

  #if 0
  auto t = Terminal::New(/* on read */[&](const Terminal::Buffer & b){
      std::copy(b.begin(),
          std::find(b.begin(), b.end(), '\0'),
          std::back_inserter(buffer));
      repaint = true;
      });
  #endif
  auto t = Terminal::New(&screen);
  const int fd = t->childfd();
  Terminal & terminal = poller.add(fd, std::move(t));

  poller.add(connection.fd(), std::unique_ptr<Events>(new WaylandPoller(connection, [&]() {
    screen.repaint();
  })));

  connection.onKeyPress = [&](const char * const key) {
    terminal.write(key, strlen(key));
  };

  poller.on();
  poller.poll();

  return 0;
}
