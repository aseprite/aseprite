// Aseprite Network Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "base/string.h"
#include "net/http_headers.h"

#include <algorithm>

namespace net {

void HttpHeaders::setHeader(const std::string& name, const std::string& value)
{
  m_map[name] = value;
}

std::string HttpHeaders::getHeader(const std::string& name) const
{
  auto lowerName = base::string_to_lower(name);
  const auto it =
    std::find_if(m_map.begin(), m_map.end(), [&lowerName](const auto& header) -> bool {
      return lowerName == base::string_to_lower(header.first);
    });
  if (it != m_map.end()) {
    return it->second;
  }
  return "";
}

} // namespace net
