C_FLAGS += -MMD

CXX_FLAGS += -MMD
CXX_FLAGS += -std=c++20

#debug 
CC_FLAGS += -DDEBUG
CC_FLAGS += -Og
CC_FLAGS += -fno-omit-frame-pointer
CC_FLAGS += -g

CXX_FLAGS += -DDEBUG
CXX_FLAGS += -Og
CXX_FLAGS += -fno-omit-frame-pointer
CXX_FLAGS += -g

#release
#CC_FLAGS += -DNDEBUG
#CC_FLAGS += -O2
#CXX_FLAGS += -DNDEBUG
#CXX_FLAGS += -O2

LIBS := -lwayland-client -lwayland-egl -lwayland-cursor -lxkbcommon -lEGL -lGL

CXX_FLAGS += $(shell pkgconf --cflags freetype2)
LIBS += $(shell pkgconf --libs freetype2)

TARGET = main
HEADERS = $(wildcard *.h)
SOURCES = $(wildcard *.cc)
C_SOURCES = $(wildcard *.c)
OBJECTS = $(patsubst %.cc,%.o,$(SOURCES))
OBJECTS += $(patsubst %.c,%.o,$(C_SOURCES))
DEPENDENCIES = $(patsubst %.o,%.d,$(OBJECTS))

$(TARGET): $(OBJECTS) $(HEADERS)
	$(CXX) $(CXX_FLAGS) $(LD_FLAGS) $(LIBS) -o $@ $(OBJECTS);

%.o : %.c %.h
	$(CC) $(CC_FLAGS) -c -o $@ $<;

%.o : %.cc %.h
	$(CXX) $(CXX_FLAGS) -c -o $@ $<;

%.o : %.cc
	$(CXX) $(CXX_FLAGS) -c -o $@ $<;

-include $(DEPENDENCIES)

clean:
	rm -v $(OBJECTS) $(TARGET) $(DEPENDENCIES);
