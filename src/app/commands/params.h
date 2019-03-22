// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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

    bool empty() const { return m_params.empty(); }
    int size() const { return int(m_params.size()); }
    void clear() { return m_params.clear(); }

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

    std::string get(const char* name) const {
      auto it = m_params.find(name);
      if (it != m_params.end())
        return it->second;
      else
        return std::string();
    }

    void operator|=(const Params& params) {
      for (const auto& p : params)
        m_params[p.first] = p.second;
    }

    template<typename T>
    const T get_as(const char* name) const {
      T value = T();
      auto it = m_params.find(name);
      if (it != m_params.end()) {
        std::istringstream stream(it->second);
        stream >> value;
      }
      return value;
    }

  private:
    Map m_params;
  };

  template<>
  inline const bool Params::get_as<bool>(const char* name) const {
    bool value = false;
    auto it = m_params.find(name);
    if (it != m_params.end()) {
      value = (it->second == "1" ||
               it->second == "true");
    }
    return value;
  }

} // namespace app

#endif
