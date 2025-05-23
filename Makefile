CXX_FLAGS := -std=c++20 -g
CXX_FLAGS += $(shell pkgconf --cflags freetype2)

LIBS := -lwayland-client -lwayland-egl -lwayland-cursor -lxkbcommon -lEGL -lGL
LIBS += $(shell pkgconf --libs freetype2)

main: main.cc buffer.cc freetype.cc freetype.h font.cc font.h poller.cc poller.h rune.h rune.cc screen.h screen.cc vt100.cc vt100.h terminal.cc terminal.h xdg-shell.c xdg-shell.h wayland.cc wayland.h
	$(CC) $(CC_FLAGS) -c -o xdg-shell.o xdg-shell.c;
	$(CXX) $(CXX_FLAGS) -c -o buffer.o buffer.cc;
	$(CXX) $(CXX_FLAGS) -c -o font.o font.cc;
	$(CXX) $(CXX_FLAGS) -c -o freetype.o freetype.cc;
	$(CXX) $(CXX_FLAGS) -c -o main.o main.cc;
	$(CXX) $(CXX_FLAGS) -c -o poller.o poller.cc;
	$(CXX) $(CXX_FLAGS) -c -o rune.o rune.cc;
	$(CXX) $(CXX_FLAGS) -c -o screen.o screen.cc;
	$(CXX) $(CXX_FLAGS) -c -o terminal.o terminal.cc;
	$(CXX) $(CXX_FLAGS) -c -o vt100.o vt100.cc;
	$(CXX) $(CXX_FLAGS) -c -o wayland.o wayland.cc;
	$(CXX) $(CXX_FLAGS) $(LD_FLAGS) $(LIBS) -o $@ buffer.o freetype.o font.o main.o poller.o rune.o screen.o terminal.o vt100.o xdg-shell.o wayland.o;

clean:
	rm -v main *.o
