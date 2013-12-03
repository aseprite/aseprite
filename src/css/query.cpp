// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/query.h"

namespace css {

void Query::addRuleValue(const std::string& ruleName, Style* style)
{
  m_ruleValue.add(ruleName, style);
}

void Query::addFromStyle(Style* style)
{
  for (Style::iterator it = style->begin(), end = style->end();
       it != end; ++it) {
    addRuleValue(it->first, style);
  }
}
  
} // namespace css
