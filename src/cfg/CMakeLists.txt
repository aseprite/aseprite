# Aseprite Config Library
# Copyright (c) 2014, 2015 David Capello

add_library(cfg-lib cfg.cpp)

target_link_libraries(cfg-lib
  ${SIMPLEINI_LIBRARY})
