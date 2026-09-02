cmake_minimum_required(VERSION 3.16)
project(floppyfish CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRCS
    main.cpp
    floppyfish.cpp
    floppyfish_common.cpp
    floppyfish_galaxy.cpp
    floppyfish_atlantis.cpp
    floppyfish_cave.cpp
    floppyfish_aquarium.cpp
    floppyfish_reef.cpp
    floppyfish_ship.cpp
    floppyfish_sound.cpp
    floppyfish_rainbow.cpp
    floppyfish_dino.cpp
    floppyfish_antarctic.cpp
    floppyfish_swamp.cpp
    floppyfish_party.cpp
    floppyfish_volcanic.cpp
    floppyfish_shark.cpp
)

find_package(PkgConfig REQUIRED)

# SDL2 + Cairo via pkg-config
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(CAIRO REQUIRED cairo)

# Common compile options
set(CXXFLAGS_COMMON -O2 -Wall -DFLOPPYSOUND)

# -----------------------
# Linux / native build
# -----------------------
if(UNIX AND NOT MINGW)
    add_executable(floppyfish ${SRCS})

    target_include_directories(floppyfish PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
    )

    target_compile_options(floppyfish PRIVATE
        ${CXXFLAGS_COMMON}
        ${SDL2_CFLAGS_OTHER}
        ${CAIRO_CFLAGS_OTHER}
    )

    target_link_libraries(floppyfish PRIVATE
        ${SDL2_LIBRARIES}
        ${CAIRO_LIBRARIES}
        m
    )

    # Debug (ASan/UBSan)
    add_executable(floppyfish_debug ${SRCS})

    target_include_directories(floppyfish_debug PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
    )

    target_compile_options(floppyfish_debug PRIVATE
        -std=c++17 -Wall -DFLOPPYSOUND
        ${SDL2_CFLAGS_OTHER}
        ${CAIRO_CFLAGS_OTHER}
        -DDEBUG -g -O0 -fno-omit-frame-pointer
        -fsanitize=address -fsanitize=undefined
        -fno-sanitize-recover=address,undefined
    )

    target_link_libraries(floppyfish_debug PRIVATE
        ${SDL2_LIBRARIES}
        ${CAIRO_LIBRARIES}
        m
        -fsanitize=address -fsanitize=undefined -rdynamic
    )
endif()

# -----------------------
# Windows (MinGW) build
# -----------------------
if(MINGW)
    add_executable(floppyfish ${SRCS})

    target_include_directories(floppyfish PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
    )

    target_compile_options(floppyfish PRIVATE
        ${CXXFLAGS_COMMON}
        -DWIN32 -D_WIN32
        ${SDL2_CFLAGS_OTHER}
        ${CAIRO_CFLAGS_OTHER}
    )

    target_link_libraries(floppyfish PRIVATE
        ${SDL2_LIBRARIES}
        ${CAIRO_LIBRARIES}
        m
        -static-libgcc -static-libstdc++
    )

    add_executable(floppyfish_debug ${SRCS})

    target_include_directories(floppyfish_debug PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${CAIRO_INCLUDE_DIRS}
    )

    target_compile_options(floppyfish_debug PRIVATE
        -std=c++17 -Wall -DFLOPPYSOUND
        ${SDL2_CFLAGS_OTHER}
        ${CAIRO_CFLAGS_OTHER}
        -DWIN32 -D_WIN32 -DDEBUG -g -O0
    )

    target_link_libraries(floppyfish_debug PRIVATE
        ${SDL2_LIBRARIES}
        ${CAIRO_LIBRARIES}
        m
        -static-libgcc -static-libstdc++
    )
endif()

