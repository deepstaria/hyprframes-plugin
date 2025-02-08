# Compiler and flags
CXX       := g++
CXXFLAGS  := -std=c++2b -O2 -g 
#CXXFLAGS 	+= -DDEBUG
SOFLAGS   := -shared -fPIC --no-gnu-unique
PKGCONFIG := pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon

# Collect sources and pkg-config flags
SOURCES   := $(wildcard *.cpp)
INCLUDES  := $(shell pkg-config --cflags $(PKGCONFIG))

# Output library name
TARGET    := libhyprframes.so
INSTALL   := /usr/lib/hyprland-plugins/

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOFLAGS) $(INCLUDES) -o $@ $^

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) $(INSTALL)
