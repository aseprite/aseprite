// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/query.h"

namespace css {

void Query::addFromStyle(const Style* style)
{
  for (Style::const_iterator it = style->begin(), end = style->end();
       it != end; ++it) {
    addRuleValue(it->first, style);
  }
}

void Query::addRuleValue(const std::string& ruleName, const Style* style)
{
  if (!m_ruleValue.exists(ruleName))
    m_ruleValue.add(ruleName, style);
}
  
} // namespace css
