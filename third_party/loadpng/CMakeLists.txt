# ASEPRITE
# Copyright (C) 2001-2013, 2015  David Capello

add_library(loadpng
  loadpng.c
  savepng.c
  regpng.c)

target_link_libraries(loadpng
  ${PNG_LIBRARIES}
  ${ZLIB_LIBRARIES})
