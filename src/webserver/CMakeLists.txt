# ASEPRITE
# Copyright (C) 2001-2013, 2015  David Capello

add_library(webserver-lib
  webserver.cpp)

target_link_libraries(webserver-lib
  ${MONGOOSE_LIBRARY})
