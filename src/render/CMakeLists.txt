# Aseprite Render Library
# Copyright (C) 2001-2017 David Capello

add_library(render-lib
  get_sprite_pixel.cpp
  gradient.cpp
  ordered_dither.cpp
  quantization.cpp
  render.cpp
  zoom.cpp)

target_link_libraries(render-lib
  doc-lib
  laf-gfx
  laf-base)
