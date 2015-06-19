# Aseprite Scripting Library
# Copyright (C) 2015  David Capello

include_directories(${DUKTAPE_DIR})

add_library(scripting-lib
  engine.cpp)
