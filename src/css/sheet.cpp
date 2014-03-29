// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/sheet.h"

#include "css/compound_style.h"
#include "css/query.h"
#include "css/stateful_style.h"

#include <stdio.h>

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

const Style* Sheet::getStyle(const std::string& name)
{
  return m_styles[name];
}

Query Sheet::query(const StatefulStyle& compound)
{
  const Style* firstStyle = &compound.style();
  const Style* style;
  Query query;
  std::string name;

  // We create a string with all states. This is the style with
  // highest priority.
  std::string states;
  for (States::const_iterator
         state_it = compound.states().begin(),
         state_end = compound.states().end(); state_it != state_end; ++state_it) {
    states += StatefulStyle::kSeparator;
    states += (*state_it)->name();
  }

  // Query by priority for the following styles:
  // style:state1:state2:...
  // ...
  // base1:state1:state2:...
  // base0:state1:state2:...
  for (style=firstStyle; style != NULL; style=style->base()) {
    name = style->name();
    name += states;

    const Style* style2 = m_styles[name];
    if (style2)
      query.addFromStyle(style2);
  }

  // Query for:
  // style:state2
  // style:state1
  // ...
  // base1:state2
  // base1:state1
  // base0:state2
  // base0:state1
  for (States::const_reverse_iterator
         state_it = compound.states().rbegin(),
         state_end = compound.states().rend(); state_it != state_end; ++state_it) {
    for (style=firstStyle; style != NULL; style=style->base()) {
      name = style->name();
      name += StatefulStyle::kSeparator;
      name += (*state_it)->name();

      const Style* style2 = m_styles[name];
      if (style2)
        query.addFromStyle(style2);
    }
  }

  // Query for:
  // style
  // ...
  // base1
  // base0
  for (style=firstStyle; style != NULL; style=style->base()) {
    query.addFromStyle(style);
  }

  return query;
}

CompoundStyle Sheet::compoundStyle(const std::string& name)
{
  return CompoundStyle(this, name);
}

} // namespace css
