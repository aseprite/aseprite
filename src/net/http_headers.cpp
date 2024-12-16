// Aseprite Network Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "net/http_headers.h"

namespace net {

void HttpHeaders::setHeader(const std::string& name, const std::string& value)
{
  m_map[name] = value;
}

} // namespace net
