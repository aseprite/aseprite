// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/compound_style.h"

#include "css/sheet.h"

namespace css {

CompoundStyle::CompoundStyle(Sheet* sheet, const std::string& name) :
  m_sheet(sheet),
  m_name(name)
{
  update();
}

void CompoundStyle::update()
{
  deleteQueries();

  const Style* style = m_sheet->getStyle(m_name);
  if (style)
    m_normal = m_sheet->query(*style);
}

CompoundStyle::~CompoundStyle()
{
  deleteQueries();
}

void CompoundStyle::deleteQueries()
{
  for (QueriesMap::iterator it = m_queries.begin(), end = m_queries.end();
       it != end; ++it) {
    delete it->second;
  }
  m_queries.clear();
}

const Value& CompoundStyle::operator[](const Rule& rule) const
{
  return m_normal[rule];
}

const Query& CompoundStyle::operator[](const States& states) const
{
  QueriesMap::const_iterator it = m_queries.find(states);

  if (it != m_queries.end())
    return *it->second;
  else {
    const Style* style = m_sheet->getStyle(m_name);
    if (style == NULL)
      return m_normal;

    Query* newQuery = new Query(m_sheet->query(StatefulStyle(*style, states)));
    m_queries[states] = newQuery;
    return *newQuery;
  }
}

} // namespace css
