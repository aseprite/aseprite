// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/sheet.h"

#include "css/compound_style.h"
#include "css/query.h"
#include "css/stateful_style.h"

namespace css {

Sheet::Sheet()
{
}

void Sheet::addRule(Rule* rule)
{
  m_rules.add(rule->name(), rule);
}

void Sheet::addStyle(Style* style)
{
  m_styles.add(style->name(), style);
}

Style* Sheet::getStyle(const std::string& name)
{
  return m_styles[name];
}

Query Sheet::query(const StatefulStyle& compound)
{
  Query query;

  std::string name = compound.style().name();
  Style* style = m_styles[name];
  Style* base = NULL;
  if (style) {
    base = style->base();
    if (base)
      query.addFromStyle(base);

    query.addFromStyle(style);
  }

  for (States::const_iterator it = compound.states().begin(),
         end = compound.states().end(); it != end; ++it) {
    if (base) {
      name = base->name();
      name += StatefulStyle::kSeparator;
      name += (*it)->name();
      style = m_styles[name];
      if (style)
        query.addFromStyle(style);
    }

    name = compound.style().name();
    name += StatefulStyle::kSeparator;
    name += (*it)->name();
    style = m_styles[name];
    if (style)
      query.addFromStyle(style);
  }

  return query;
}

CompoundStyle Sheet::compoundStyle(const std::string& name)
{
  return CompoundStyle(this, name);
}

} // namespace css
