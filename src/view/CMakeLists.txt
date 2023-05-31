# Aseprite View Library
# Copyright (c) 2023  Igara Studio S.A.

add_library(view-lib
  cels.cpp
  layers.cpp
  range.cpp)

target_link_libraries(view-lib
  doc-lib
  laf-base)
