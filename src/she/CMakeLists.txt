# SHE
# Copyright (C) 2012-2014  David Capello

set(SHE_SOURCES
  alleg4/fontbmp.cpp
  alleg4/she_alleg4.cpp)

if(APPLE)
  if(NOT USE_SHARED_ALLEGRO4)
    list(APPEND SHE_SOURCES alleg4/app.mm)
  endif()

  list(APPEND SHE_SOURCES osx/logger.mm)
endif()

add_library(she ${SHE_SOURCES})
