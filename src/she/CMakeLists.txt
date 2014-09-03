# SHE
# Copyright (C) 2012-2014  David Capello

set(SHE_SOURCES
  alleg4/clock.cpp
  alleg4/close_button.cpp
  alleg4/fontbmp.cpp
  alleg4/key_poller.cpp
  alleg4/mouse_poller.cpp
  alleg4/she.cpp)

if(APPLE)
  if(NOT USE_SHARED_ALLEGRO4)
    list(APPEND SHE_SOURCES alleg4/app.mm)
  endif()

  list(APPEND SHE_SOURCES osx/logger.mm)
endif()

add_library(she ${SHE_SOURCES})
