CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Isrc -DFLOPPYSOUND
LIBS     := $(shell pkg-config --cflags --libs sdl2 cairo) -lm

SRCS := main.cpp floppyfish.cpp floppyfish_atlantis.cpp \
     floppyfish_cave.cpp floppyfish_common.cpp \
     floppyfish_reef.cpp floppyfish_ship.cpp

BIN  := floppyfish

$(BIN): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(BIN) $(LIBS) 

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)

.PHONY: run clean
