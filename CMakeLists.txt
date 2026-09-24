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
  src/core/metrics.cpp
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

# --- tests ---
enable_testing()
foreach(t test_grid test_metrics test_png)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE cave_core cave_io)
  add_test(NAME ${t} COMMAND ${t})
endforeach()
