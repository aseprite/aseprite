# Aseprite Desktop Integration Module
# Copyright (C) 2016  Gabriel Rauter
# Copyright (C) 2016  David Capello
#
# Licensed under the the MIT License (https://opensource.org/licenses/MIT).

if(UNIX AND NOT APPLE)
  # Desktop shortcut
  install(FILES aseprite.desktop
          DESTINATION share/applications)

  # GNOME Thumbnailer
  install(FILES mime/aseprite.xml
          DESTINATION share/mime/packages)
  install(PROGRAMS aseprite-thumbnailer
          DESTINATION bin)
  install(FILES gnome/aseprite.thumbnailer
          DESTINATION share/thumbnailers)

  # Qt Thumbnailer
  if(WITH_QT_THUMBNAILER)
    add_subdirectory(kde)
  endif()
endif()
