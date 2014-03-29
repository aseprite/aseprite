// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CSS_QUERY_H_INCLUDED
#define CSS_QUERY_H_INCLUDED
#pragma once

#include "css/rule.h"
#include "css/style.h"
#include "css/value.h"

#include <map>

namespace css {

  class Query {
  public:
    Query() { }

    // Adds more rules from the given style only if the query doesn't
    // contain those rules already.
    void addFromStyle(const Style* style);

    const Value& operator[](const Rule& rule) const {
      const Style* style = m_ruleValue[rule.name()];
      if (style)
        return (*style)[rule.name()];
      else
        return m_none;
    }

  private:
    void addRuleValue(const std::string& ruleName, const Style* style);

    Styles m_ruleValue;
    Value m_none;
  };

} // namespace css

#endif
