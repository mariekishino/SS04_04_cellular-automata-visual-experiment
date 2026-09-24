cmake_minimum_required(VERSION 3.16)
project(cave LANGUAGES CXX)
# cave = cellular-automata-visual-experiment

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# --- code_version: git hash + dirty flag, injected at configure time ---
find_package(Git QUIET)
set(CAVE_GIT_HASH "unknown")
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _hash OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} status --porcelain --untracked-files=no
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _dirty OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  if(_hash)
    set(CAVE_GIT_HASH "${_hash}")
    if(_dirty)
      set(CAVE_GIT_HASH "${_hash}-dirty")
    endif()
  endif()
endif()
message(STATUS "code_version = ${CAVE_GIT_HASH}")

# --- core library: simulation only, no I/O, no rendering ---
add_library(cave_core
  src/core/grid.cpp
  src/core/model.cpp
  src/core/stimulus.cpp
  src/core/lenia.cpp
  src/core/gray_scott.cpp
  src/core/metrics.cpp
  src/core/presets.cpp
)
target_include_directories(cave_core PUBLIC src)
target_compile_options(cave_core PRIVATE -Wall -Wextra -Wpedantic)

# --- io library: PNG writer, CSV, checkpoint ---
add_library(cave_io
  src/io/png.cpp
  src/io/colormap.cpp
)
target_link_libraries(cave_io PUBLIC cave_core)
target_compile_options(cave_io PRIVATE -Wall -Wextra -Wpedantic)

# --- app ---
add_executable(cave src/app/main.cpp)
target_link_libraries(cave PRIVATE cave_core cave_io)
target_compile_definitions(cave PRIVATE CAVE_GIT_HASH="${CAVE_GIT_HASH}")
target_compile_options(cave PRIVATE -Wall -Wextra -Wpedantic)

# --- optional SDL3 window app (Phase 1) ---
option(CAVE_WITH_SDL3 "Build cave_window, the SDL3 window app" OFF)
if(CAVE_WITH_SDL3)
  find_package(SDL3 CONFIG QUIET)
  if(NOT SDL3_FOUND)
    message(STATUS "SDL3 not found on the system; fetching release-3.4.16 and building it statically")
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
    # Phase 1 needs only video. Audio comes back in Phase 2.
    set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
    set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
    set(SDL_GPU OFF CACHE BOOL "" FORCE)
    set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
    set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
    set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG release-3.4.16
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(SDL3)
  endif()
  add_executable(cave_window src/app/window_main.cpp)
  target_link_libraries(cave_window PRIVATE cave_core cave_io SDL3::SDL3)
  target_compile_options(cave_window PRIVATE -Wall -Wextra -Wpedantic)
endif()

# --- tests ---
enable_testing()
foreach(t test_grid test_lenia test_gray_scott test_metrics test_shape test_png test_determinism test_stimulus)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE cave_core cave_io)
  add_test(NAME ${t} COMMAND ${t})
endforeach()
