# Aseprite Desktop Integration Module
# Copyright (C) 2016  Gabriel Rauter
#
# Licensed under the the MIT License (https://opensource.org/licenses/MIT).

if(UNIX AND NOT APPLE)
  install(FILES mime/aseprite.xml
          DESTINATION share/mime/packages)
  install(PROGRAMS aseprite-thumbnailer
          DESTINATION bin)
  install(FILES gnome/aseprite.thumbnailer
          DESTINATION share/thumbnailers)
  if(WITH_QT_THUMBNAILER)
    add_subdirectory(kde)
  endif()
endif()
