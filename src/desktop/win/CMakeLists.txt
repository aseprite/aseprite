# Desktop Integration
# Copyright (C) 2017-2018 David Capello

add_library(aseprite-thumbnailer SHARED
  dllmain.cpp
  exports.def
  thumbnail_handler.cpp)

target_link_libraries(aseprite-thumbnailer
  laf-base
  dio-lib
  render-lib
  shlwapi)
