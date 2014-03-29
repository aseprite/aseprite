// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CSS_COMPOUND_STYLE_H_INCLUDED
#define CSS_COMPOUND_STYLE_H_INCLUDED
#pragma once

#include "css/query.h"
#include "css/rule.h"
#include "css/state.h"
#include "css/stateful_style.h"

namespace css {

  class Sheet;
 
  class CompoundStyle {
  public:
    CompoundStyle(Sheet* sheet, const std::string& name);
    ~CompoundStyle();

    void update();

    const Value& operator[](const Rule& rule) const;
    const Query& operator[](const States& states) const;

  private:
    typedef std::map<States, Query*> QueriesMap;

    void deleteQueries();

    Sheet* m_sheet;
    std::string m_name;
    Query m_normal;
    mutable QueriesMap m_queries;
  };

} // namespace css

#endif
