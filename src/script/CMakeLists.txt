# Aseprite Scripting Library
# Copyright (C) 2015-2016  David Capello

include_directories(${DUKTAPE_DIR})

add_library(duktape ${DUKTAPE_DIR}/duktape.c)
add_library(script-lib engine.cpp engine_delegate.cpp)

if(UNIX)
  target_link_libraries(duktape m)
endif()

target_link_libraries(script-lib duktape)
