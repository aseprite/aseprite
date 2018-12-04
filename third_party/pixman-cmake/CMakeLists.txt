# Copyright (C) 2014, 2016 by David Capello

add_definitions(-DHAVE_CONFIG_H)

if(UNIX OR APPLE)
  add_definitions(-DHAVE_PTHREADS)
endif()

# Minor and micro versions must be even numbers (to avoid defining DEBUG)
set(PIXMAN_VERSION_MAJOR 0)
set(PIXMAN_VERSION_MINOR 34)
set(PIXMAN_VERSION_MICRO 0)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../pixman/pixman/pixman-version.h.in"
  "${CMAKE_BINARY_DIR}/pixman-version.h")

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../pixman/pixman
  ${CMAKE_BINARY_DIR})

add_library(pixman
  ../pixman/pixman/pixman-access-accessors.c
  ../pixman/pixman/pixman-access.c
  ../pixman/pixman/pixman-arm.c
  ../pixman/pixman/pixman-bits-image.c
  ../pixman/pixman/pixman-combine-float.c
  ../pixman/pixman/pixman-combine32.c
  ../pixman/pixman/pixman-conical-gradient.c
  ../pixman/pixman/pixman-edge-accessors.c
  ../pixman/pixman/pixman-edge.c
  ../pixman/pixman/pixman-fast-path.c
  ../pixman/pixman/pixman-filter.c
  ../pixman/pixman/pixman-general.c
  ../pixman/pixman/pixman-glyph.c
  ../pixman/pixman/pixman-gradient-walker.c
  ../pixman/pixman/pixman-image.c
  ../pixman/pixman/pixman-implementation.c
  ../pixman/pixman/pixman-linear-gradient.c
  ../pixman/pixman/pixman-matrix.c
  ../pixman/pixman/pixman-mips.c
  ../pixman/pixman/pixman-noop.c
  ../pixman/pixman/pixman-ppc.c
  ../pixman/pixman/pixman-radial-gradient.c
  ../pixman/pixman/pixman-region16.c
  ../pixman/pixman/pixman-region32.c
  ../pixman/pixman/pixman-solid-fill.c
  ../pixman/pixman/pixman-timer.c
  ../pixman/pixman/pixman-trap.c
  ../pixman/pixman/pixman-utils.c
  ../pixman/pixman/pixman-x86.c
  ../pixman/pixman/pixman.c)
