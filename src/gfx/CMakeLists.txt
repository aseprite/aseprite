# Aseprite
# Copyright (C) 2001-2017  David Capello

add_library(gfx-lib
  hsl.cpp
  hsv.cpp
  packing_rects.cpp
  region.cpp
  rgb.cpp)

target_link_libraries(gfx-lib
  laf-base
  ${PIXMAN_LIBRARY})
