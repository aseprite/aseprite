// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef CSS_QUERY_H_INCLUDED
#define CSS_QUERY_H_INCLUDED

#include "css/rule.h"
#include "css/style.h"
#include "css/value.h"

#include <map>

namespace css {

  class Query {
  public:
    Query() { }

    void addRuleValue(const std::string& ruleName, Style* style);
    void addFromStyle(Style* style);

    const Value& operator[](const Rule& rule) const {
      Style* style = m_ruleValue[rule.name()];
      if (style)
        return (*style)[rule.name()];
      else
        return m_none;
    }

  private:
    Styles m_ruleValue;
    Value m_none;
  };

} // namespace css

#endif
