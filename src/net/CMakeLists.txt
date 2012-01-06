# ASEPRITE
# Copyright (C) 2001-2012  David Capello

add_definitions(-DCURL_STATICLIB)

add_library(net-lib
  http_headers.cpp
  http_request.cpp
  http_response.cpp)
