# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2011  David Capello

set(UPDATER_LIB_SOURCES
  check_update.cpp
  user_agent.cpp)

if(WIN32)
  set(UPDATER_LIB_SOURCES ${UPDATER_LIB_SOURCES} user_agent_win.c)
else()
  if(APPLE)
    set(UPDATER_LIB_SOURCES ${UPDATER_LIB_SOURCES} user_agent_mac.m)
  endif()
endif()

add_library(updater-lib ${UPDATER_LIB_SOURCES})
