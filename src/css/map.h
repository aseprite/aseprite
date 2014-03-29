// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CSS_MAP_H_INCLUDED
#define CSS_MAP_H_INCLUDED
#pragma once

#include <map>
#include <string>

namespace css {

  template<typename T>
  class Map {
  public:
    typedef std::map<std::string, T> map;
    typedef typename map::iterator iterator;
    typedef typename map::const_iterator const_iterator;

    Map() : m_default() { }

    iterator begin() { return m_map.begin(); }
    iterator end() { return m_map.end(); }

    const_iterator begin() const { return m_map.begin(); }
    const_iterator end() const { return m_map.end(); }

    const T& operator[](const std::string& name) const {
      const_iterator it = m_map.find(name);
      if (it != m_map.end())
        return it->second;
      else
        return m_default;
    }

    T& operator[](const std::string& name) {
      iterator it = m_map.find(name);
      if (it != m_map.end())
        return it->second;
      else
        return m_map[name] = T();
    }

    void add(const std::string& name, T value) {
      m_map[name] = value;
    }

    bool exists(const std::string& name) const {
      return (m_map.find(name) != m_map.end());
    }

  private:
    map m_map;
    T m_default;
  };

} // namespace css

#endif
