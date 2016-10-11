# Aseprite Render Library
# Copyright (C) 2001-2016 David Capello

add_library(render-lib
  get_sprite_pixel.cpp
  quantization.cpp
  render.cpp
  zoom.cpp)

target_link_libraries(render-lib
  doc-lib
  gfx-lib
  base-lib)
