# Aseprite
# Copyright (C) 2001-2016  David Capello

add_library(gfx-lib
  hsv.cpp
  packing_rects.cpp
  region.cpp
  rgb.cpp)

target_link_libraries(gfx-lib
  laf-base
  ${PIXMAN_LIBRARY})
