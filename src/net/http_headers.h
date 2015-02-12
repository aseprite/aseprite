// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef NET_HTTP_HEADERS_H_INCLUDED
#define NET_HTTP_HEADERS_H_INCLUDED
#pragma once

#include <map>
#include <string>

namespace net {

class HttpHeaders
{
public:
  typedef std::map<std::string, std::string> Map;
  typedef Map::iterator iterator;
  typedef Map::const_iterator const_iterator;

  iterator begin() { return m_map.begin(); }
  iterator end() { return m_map.end(); }
  const_iterator begin() const { return m_map.begin(); }
  const_iterator end() const { return m_map.end(); }

  void setHeader(const std::string& name,
                 const std::string& value);

private:
  Map m_map;
};

} // namespace net

#endif  // NET_HTTP_HEADERS_H_INCLUDED
