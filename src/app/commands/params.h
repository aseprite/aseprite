// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
