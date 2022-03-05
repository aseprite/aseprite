# Aseprite Fixmath Library
# Based on Allegro library by Shawn Hargreaves

add_library(fixmath-lib
  fixmath.cpp)

target_link_libraries(fixmath-lib
  laf-base)

target_include_directories(fixmath-lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
