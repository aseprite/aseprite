# Aseprite Desktop Integration Module
# Copyright (C) 2016  Gabriel Rauter
#
# Dual licensed under the MIT and GPL 2 licenses.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation or the MIT License
# (https://opensource.org/licenses/MIT).

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
