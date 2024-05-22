# ASEPRITE
# Copyright (C) 2020-2024  Igara Studio S.A.
# Copyright (C) 2001-2017  David Capello

# By default the updater-lib will contain only the functions related
# the user agent string.
add_library(updater-lib user_agent.cpp)
target_link_libraries(updater-lib laf-base ver-lib)

# Only when ENABLE_UPDATER is ON we'll enable the "check for update"
# portion of the library.
if(ENABLE_UPDATER)
  target_sources(updater-lib PRIVATE check_update.cpp)
  target_link_libraries(updater-lib net-lib ${TINYXML_LIBRARY})
endif()
