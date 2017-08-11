# Aseprite Scripting Library
# Copyright (C) 2015-2017  David Capello

add_library(script-lib engine.cpp engine_delegate.cpp)

target_link_libraries(script-lib
  laf-base
  mujs)
