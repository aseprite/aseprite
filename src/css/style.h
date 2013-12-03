// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef CSS_STYLE_H_INCLUDED
#define CSS_STYLE_H_INCLUDED

#include <map>
#include <string>

#include "css/map.h"
#include "css/rule.h"
#include "css/value.h"

namespace css {

  class Style {
  public:
    typedef Values::iterator iterator;

    Style() { }
    Style(const std::string& name, Style* base = NULL);

    const std::string& name() const { return m_name; }
    Style* base() const { return m_base; }

    const Value& operator[](const Rule& rule) const {
      return m_values[rule.name()];
    }

    Value& operator[](const Rule& rule) {
      return m_values[rule.name()];
    }

    iterator begin() { return m_values.begin(); }
    iterator end() { return m_values.end(); }

  private:
    std::string m_name;
    Style* m_base;
    Values m_values;
  };

  typedef Map<Style*> Styles;

} // namespace css

#endif
