# Desktop Integration
# Copyright (C) 2017-2018 David Capello

# Windows
if(WIN32)
  add_subdirectory(win)
endif()

# Linux-like
if(UNIX AND NOT APPLE)
  add_subdirectory(linux)
endif()
