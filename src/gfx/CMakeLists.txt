# Aseprite
# Copyright (C) 2001-2015  David Capello

add_library(gfx-lib
  clip.cpp
  hsv.cpp
  packing_rects.cpp
  region.cpp
  rgb.cpp
  transformation.cpp)

target_link_libraries(gfx-lib
  fixmath-lib
  ${PIXMAN_LIBRARY})
