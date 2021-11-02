# ASEPRITE
# Copyright (C) 2020-2021  Igara Studio S.A.
# Copyright (C) 2001-2017  David Capello

set(UPDATER_LIB_SOURCES
  check_update.cpp
  user_agent.cpp)

add_library(updater-lib ${UPDATER_LIB_SOURCES})

target_link_libraries(updater-lib
  laf-base
  net-lib
  ver-lib
  ${TINYXML_LIBRARY})
