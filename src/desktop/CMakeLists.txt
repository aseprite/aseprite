# Desktop Integration
# Copyright (C) 2022 Igara Studio S.A.
# Copyright (C) 2017-2018 David Capello

if(WIN32)
  # Thumbnails for Windows File Explorer
  add_subdirectory(win)
elseif(APPLE)
  # QuickLook preview / macOS Finder
  add_subdirectory(osx)
else(UNIX)
  # Linux-like
  add_subdirectory(linux)
endif()
