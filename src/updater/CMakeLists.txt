# ASEPRITE
# Copyright (C) 2020  Igara Studio S.A.
# Copyright (C) 2001-2017  David Capello

set(UPDATER_LIB_SOURCES
  check_update.cpp
  user_agent.cpp)

if(WIN32)
  set(UPDATER_LIB_SOURCES ${UPDATER_LIB_SOURCES} user_agent_win.c)
elseif(APPLE)
  set(UPDATER_LIB_SOURCES ${UPDATER_LIB_SOURCES} user_agent_mac.mm)
endif()

add_library(updater-lib ${UPDATER_LIB_SOURCES})

target_link_libraries(updater-lib
  net-lib
  cfg-lib
  ver-lib
  ${TINYXML_LIBRARY})
