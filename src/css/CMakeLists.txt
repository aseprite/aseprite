# Aseprite CSS Library
# Copyright (C) 2013-2016 David Capello

add_library(css-lib
  compound_style.cpp
  query.cpp
  rule.cpp
  sheet.cpp
  style.cpp
  value.cpp)

target_link_libraries(css-lib
  laf-base)
