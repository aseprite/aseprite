# Aseprite FreeType Wrapper
# Copyright (C) 2017 David Capello

add_library(ft-lib
  lib.cpp
  stream.cpp)

target_link_libraries(ft-lib
  laf-base
  ${FREETYPE_LIBRARIES})
