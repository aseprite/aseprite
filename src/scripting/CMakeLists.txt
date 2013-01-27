# ASEPRITE
# Copyright (C) 2001-2013  David Capello

find_package(V8)

set(V8_FOUND ${V8_FOUND} PARENT_SCOPE)
set(V8_INCLUDE_DIR ${V8_INCLUDE_DIR} PARENT_SCOPE)
set(V8_LIBRARIES ${V8_LIBRARIES} PARENT_SCOPE)

if(V8_FOUND)
  include_directories(${V8_INCLUDE_DIR})
  add_definitions(-DHAVE_V8_LIBRARY)
else()
  message(WARNING "V8 not found, compiling without JS scripting support")
endif()

add_library(scripting-lib
  engine.cpp)
