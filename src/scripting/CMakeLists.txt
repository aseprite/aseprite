# ASEPRITE
# Copyright (C) 2001-2013, 2015  David Capello

add_library(scripting-lib
  engine.cpp)

target_link_libraries(scripting-lib duktape)
