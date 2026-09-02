cmake_minimum_required(VERSION 3.20)
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

# ---------------------------------------------------------
# Linux: pkg-config
# ---------------------------------------------------------
if(UNIX AND NOT WIN32)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(CAIRO REQUIRED cairo)
endif()

# ---------------------------------------------------------
# Windows: Visual Studio (MSVC)
# ---------------------------------------------------------
if(MSVC)
    # SDL2 ships a proper CMake config via vcpkg
    find_package(SDL2 REQUIRED CONFIG)
    # Cairo's vcpkg port only ships pkg-config files (no CMake config),
    # so use PkgConfig here too -- this works for both x64-windows and
    # arm64-windows vcpkg triplets as long as CMAKE_TOOLCHAIN_FILE points
    # at vcpkg.cmake (vcpkg supplies pkgconf.exe automatically).
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
endif()

# ---------------------------------------------------------
# MinGW fallback
# ---------------------------------------------------------
if(MINGW)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(CAIRO REQUIRED cairo)
endif()

# ---------------------------------------------------------
# Executables
# ---------------------------------------------------------
add_executable(floppyfish WIN32 ${SRCS})
add_executable(floppyfish_debug WIN32 ${SRCS})

# ---------------------------------------------------------
# Linux flags
# ---------------------------------------------------------
if(UNIX AND NOT WIN32)
    target_include_directories(floppyfish PRIVATE
        ${SDL2_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS}
    )
    target_compile_options(floppyfish PRIVATE
        -O2 -Wall -DFLOPPYSOUND
        ${SDL2_CFLAGS_OTHER} ${CAIRO_CFLAGS_OTHER}
    )
    target_link_libraries(floppyfish PRIVATE
        ${SDL2_LIBRARIES} ${CAIRO_LIBRARIES} m
    )

    target_include_directories(floppyfish_debug PRIVATE
        ${SDL2_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS}
    )
    target_compile_options(floppyfish_debug PRIVATE
        -std=c++17 -Wall -DFLOPPYSOUND
        ${SDL2_CFLAGS_OTHER} ${CAIRO_CFLAGS_OTHER}
        -DDEBUG -g -O0 -fno-omit-frame-pointer
        -fsanitize=address -fsanitize=undefined
        -fno-sanitize-recover=address,undefined
    )
    target_link_libraries(floppyfish_debug PRIVATE
        ${SDL2_LIBRARIES} ${CAIRO_LIBRARIES} m
        -fsanitize=address -fsanitize=undefined -rdynamic
    )
endif()

# ---------------------------------------------------------
# Windows: MSVC (Visual Studio)
# ---------------------------------------------------------
if(MSVC)
    target_compile_definitions(floppyfish PRIVATE FLOPPYSOUND)
    target_compile_definitions(floppyfish_debug PRIVATE FLOPPYSOUND DEBUG)

    target_link_libraries(floppyfish PRIVATE
        SDL2::SDL2 SDL2::SDL2main
        PkgConfig::CAIRO
    )

    target_link_libraries(floppyfish_debug PRIVATE
        SDL2::SDL2 SDL2::SDL2main
        PkgConfig::CAIRO
    )
endif()

# ---------------------------------------------------------
# Windows: MinGW
# ---------------------------------------------------------
if(MINGW)
    target_include_directories(floppyfish PRIVATE
        ${SDL2_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS}
    )
    target_compile_options(floppyfish PRIVATE
        -O2 -Wall -DFLOPPYSOUND -DWIN32 -D_WIN32
        ${SDL2_CFLAGS_OTHER} ${CAIRO_CFLAGS_OTHER}
    )
    target_link_libraries(floppyfish PRIVATE
        ${SDL2_LIBRARIES} ${CAIRO_LIBRARIES} m
        -static-libgcc -static-libstdc++
    )

    target_include_directories(floppyfish_debug PRIVATE
        ${SDL2_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS}
    )
    target_compile_options(floppyfish_debug PRIVATE
        -std=c++17 -Wall -DFLOPPYSOUND -DWIN32 -D_WIN32 -DDEBUG -g -O0
        ${SDL2_CFLAGS_OTHER} ${CAIRO_CFLAGS_OTHER}
    )
    target_link_libraries(floppyfish_debug PRIVATE
        ${SDL2_LIBRARIES} ${CAIRO_LIBRARIES} m
        -static-libgcc -static-libstdc++
    )
endif()

