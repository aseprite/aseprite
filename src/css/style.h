// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CSS_STYLE_H_INCLUDED
#define CSS_STYLE_H_INCLUDED
#pragma once

#include <map>
#include <string>

#include "css/map.h"
#include "css/rule.h"
#include "css/value.h"

namespace css {

  class Style {
  public:
    typedef Values::iterator iterator;
    typedef Values::const_iterator const_iterator;

    Style() { }
    Style(const std::string& name, const Style* base = NULL);

    const std::string& name() const { return m_name; }
    const Style* base() const { return m_base; }

    const Value& operator[](const Rule& rule) const {
      return m_values[rule.name()];
    }

    Value& operator[](const Rule& rule) {
      return m_values[rule.name()];
    }

    iterator begin() { return m_values.begin(); }
    iterator end() { return m_values.end(); }
    const_iterator begin() const { return m_values.begin(); }
    const_iterator end() const { return m_values.end(); }

  private:
    std::string m_name;
    const Style* m_base;
    Values m_values;
  };

  typedef Map<const Style*> Styles;

} // namespace css

#endif
