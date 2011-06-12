/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef NET_HTTP_HEADERS_H_INCLUDED
#define NET_HTTP_HEADERS_H_INCLUDED

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

#endif	// NET_HTTP_HEADERS_H_INCLUDED
