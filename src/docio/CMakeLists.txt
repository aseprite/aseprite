# Aseprite Document IO Library
# Copyright (c) 2016 David Capello

add_library(docio-lib
  detect_format.cpp)

target_link_libraries(docio-lib
  flic-lib
  laf-base)
