# Aseprite Document IO Library
# Copyright (c) 2022-2023 Igara Studio S.A.
# Copyright (c) 2016-2018 David Capello

add_library(dio-lib
  aseprite_common.cpp
  aseprite_decoder.cpp
  decode_file.cpp
  decoder.cpp
  detect_format.cpp
  stdio.cpp)

target_link_libraries(dio-lib
  ${ZLIB_LIBRARIES}
  fmt
  flic-lib
  laf-base
  fixmath-lib
  doc-lib)

target_include_directories(dio-lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
