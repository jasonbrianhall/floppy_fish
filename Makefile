# floppyfish Makefile
# Native Linux build with:        make linux
# Cross-compile for Windows with: make windows

CXX_LINUX = g++
CXX_WIN   = x86_64-w64-mingw32-g++

PKG_CONFIG_LINUX = pkg-config
PKG_CONFIG_WIN   = mingw64-pkg-config

CXXFLAGS_COMMON = -std=c++17 -O2 -Wall -DFLOPPYSOUND

SDL2_CFLAGS_LINUX  := $(shell $(PKG_CONFIG_LINUX) --cflags sdl2   2>/dev/null || echo "")
SDL2_LIBS_LINUX    := $(shell $(PKG_CONFIG_LINUX) --libs   sdl2   2>/dev/null || echo "-lSDL2")
CAIRO_CFLAGS_LINUX := $(shell $(PKG_CONFIG_LINUX) --cflags cairo  2>/dev/null || echo "")
CAIRO_LIBS_LINUX   := $(shell $(PKG_CONFIG_LINUX) --libs   cairo  2>/dev/null || echo "-lcairo")

SDL2_CFLAGS_WIN  := $(shell $(PKG_CONFIG_WIN) --cflags sdl2  2>/dev/null || echo "")
SDL2_LIBS_WIN    := $(shell $(PKG_CONFIG_WIN) --libs   sdl2  2>/dev/null || echo "-lmingw32 -lSDL2main -lSDL2")
CAIRO_CFLAGS_WIN := $(shell $(PKG_CONFIG_WIN) --cflags cairo 2>/dev/null || echo "")
CAIRO_LIBS_WIN   := $(shell $(PKG_CONFIG_WIN) --libs   cairo 2>/dev/null || echo "-lcairo")

CXXFLAGS_LINUX = $(CXXFLAGS_COMMON) $(SDL2_CFLAGS_LINUX) $(CAIRO_CFLAGS_LINUX)
LDFLAGS_LINUX  = $(SDL2_LIBS_LINUX) $(CAIRO_LIBS_LINUX) -lm

CXXFLAGS_WIN = $(CXXFLAGS_COMMON) $(SDL2_CFLAGS_WIN) $(CAIRO_CFLAGS_WIN) -DWIN32 -D_WIN32
LDFLAGS_WIN  = $(SDL2_LIBS_WIN) $(CAIRO_LIBS_WIN) -lm -static-libgcc -static-libstdc++

SRCS = main.cpp floppyfish.cpp floppyfish_common.cpp \
       floppyfish_atlantis.cpp floppyfish_cave.cpp \
       floppyfish_reef.cpp floppyfish_ship.cpp

BUILD_DIR      = build
BUILD_DIR_LINUX = $(BUILD_DIR)/linux
BUILD_DIR_WIN   = $(BUILD_DIR)/windows

EXECUTABLE_LINUX = floppyfish
EXECUTABLE_WIN   = floppyfish.exe

DLL_SOURCE_DIR = /usr/x86_64-w64-mingw32/sys-root/mingw/bin

OBJECTS_LINUX = $(addprefix $(BUILD_DIR_LINUX)/, $(notdir $(SRCS:.cpp=.o)))
OBJECTS_WIN   = $(addprefix $(BUILD_DIR_WIN)/,   $(notdir $(SRCS:.cpp=.win.o)))

$(shell mkdir -p $(BUILD_DIR_LINUX) $(BUILD_DIR_WIN))

# ============================================================================
# TOP-LEVEL TARGETS
# ============================================================================
.PHONY: all linux windows run clean clean-all help

all: linux

linux: $(BUILD_DIR_LINUX)/$(EXECUTABLE_LINUX)

windows: $(BUILD_DIR_WIN)/$(EXECUTABLE_WIN) win-collect-dlls

# ============================================================================
# LINUX BUILD
# ============================================================================
$(BUILD_DIR_LINUX)/$(EXECUTABLE_LINUX): $(OBJECTS_LINUX)
	@echo "Linking Linux: $@"
	$(CXX_LINUX) $(CXXFLAGS_LINUX) $^ -o $@ $(LDFLAGS_LINUX)
	@echo "done $@"

$(BUILD_DIR_LINUX)/%.o: %.cpp floppyfish_common.h visualization.h
	@echo "Compiling (Linux): $<"
	$(CXX_LINUX) $(CXXFLAGS_LINUX) -c $< -o $@

# ============================================================================
# WINDOWS BUILD
# ============================================================================
$(BUILD_DIR_WIN)/$(EXECUTABLE_WIN): $(OBJECTS_WIN)
	@echo "Linking Windows: $@"
	$(CXX_WIN) $(CXXFLAGS_WIN) $^ -o $@ $(LDFLAGS_WIN)
	@echo "done $@"

$(BUILD_DIR_WIN)/%.win.o: %.cpp floppyfish_common.h visualization.h
	@echo "Compiling (Windows): $<"
	$(CXX_WIN) $(CXXFLAGS_WIN) -c $< -o $@

# ============================================================================
# DLL COLLECTION
# ============================================================================
.PHONY: win-collect-dlls
win-collect-dlls: $(BUILD_DIR_WIN)/$(EXECUTABLE_WIN)
	@echo "Collecting Windows DLLs..."
	@if [ -f collect_dlls.sh ]; then \
		./collect_dlls.sh $(BUILD_DIR_WIN)/$(EXECUTABLE_WIN) $(DLL_SOURCE_DIR) $(BUILD_DIR_WIN); \
	else \
		echo "Tip: write collect_dlls.sh or copy manually:"; \
		echo "  SDL2.dll, libcairo-2.dll, libwinpthread-1.dll,"; \
		echo "  libgcc_s_seh-1.dll, libstdc++-6.dll, and cairo's transitive deps"; \
		echo "  (libpng16, zlib1, libfreetype, libfontconfig, libexpat, libpixman-1)"; \
	fi

# ============================================================================
# UTILITY
# ============================================================================
run: linux
	./$(BUILD_DIR_LINUX)/$(EXECUTABLE_LINUX)

clean:
	rm -f $(OBJECTS_LINUX) $(OBJECTS_WIN)
	rm -f $(BUILD_DIR_LINUX)/$(EXECUTABLE_LINUX) $(BUILD_DIR_WIN)/$(EXECUTABLE_WIN)
	rm -rf $(BUILD_DIR_WIN)/dlls

clean-all:
	rm -rf $(BUILD_DIR)

help:
	@echo "floppyfish build targets:"
	@echo "  make            - Linux build (default)"
	@echo "  make linux      - Linux build       (output: build/linux/floppyfish)"
	@echo "  make windows    - Windows cross-compile + DLL collection (output: build/windows/)"
	@echo "  make run        - Build and run the Linux binary"
	@echo "  make clean      - Remove object files and binaries"
	@echo "  make clean-all  - Remove entire build/ directory"
	@echo ""
	@echo "Windows cross-compile requires:"
	@echo "  x86_64-w64-mingw32-g++ (mingw-w64)"
	@echo "  mingw64-pkg-config"
	@echo "  mingw64 packages: sdl2, cairo"
