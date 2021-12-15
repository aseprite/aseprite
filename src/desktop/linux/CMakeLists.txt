# Desktop Integration
# Copyright (C) 2016 Gabriel Rauter

# Desktop shortcut
install(FILES aseprite.desktop DESTINATION share/applications)

# GNOME Thumbnailer
install(FILES mime/aseprite.xml DESTINATION share/mime/packages)
install(PROGRAMS aseprite-thumbnailer DESTINATION bin)
install(FILES gnome/aseprite.thumbnailer DESTINATION share/thumbnailers)

# Qt Thumbnailer
if(ENABLE_QT_THUMBNAILER)
  add_subdirectory(kde)
endif()
