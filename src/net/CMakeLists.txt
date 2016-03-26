# Aseprite Network Library
# Copyright (C) 2001-2015 David Capello

add_library(net-lib
  http_headers.cpp
  http_request.cpp
  http_response.cpp)

target_link_libraries(net-lib
  ${LIBCURL_LIBRARIES})
