/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef APP_COMMANDS_PARAMS_H
#define APP_COMMANDS_PARAMS_H

#include <map>
#include <string>
#include <sstream>

namespace app {

  class Params {
  public:
    typedef std::map<std::string, std::string> Map;
    typedef Map::iterator iterator;
    typedef Map::const_iterator const_iterator;

    iterator begin() { return m_params.begin(); }
    iterator end() { return m_params.end(); }
    const_iterator begin() const { return m_params.begin(); }
    const_iterator end() const { return m_params.end(); }

    Params() { }
    Params(const Params& copy) : m_params(copy.m_params) { }
    virtual ~Params() { }

    Params* clone() const {
      return new Params(*this);
    }

    bool empty() const {
      return m_params.empty();
    }

    bool has_param(const char* name) const {
      return m_params.find(name) != m_params.end();
    }

    bool operator==(const Params& params) const {
      return m_params == params.m_params;
    }

    bool operator!=(const Params& params) const {
      return m_params != params.m_params;
    }

    std::string& set(const char* name, const char* value) {
      return m_params[name] = value;
    }

    std::string& get(const char* name) {
      return m_params[name];
    }

    template<typename T>
    T get_as(const char* name) {
      std::istringstream stream(m_params[name]);
      T value;
      stream >> value;
      return value;
    }

  private:
    Map m_params;
  };

} // namespace app

#endif
