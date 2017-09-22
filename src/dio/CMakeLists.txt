# Aseprite Document IO Library
# Copyright (c) 2016-2017 David Capello

add_library(dio-lib
  detect_format.cpp)

target_link_libraries(dio-lib
  flic-lib
  laf-base)
