// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef CSS_SHEET_H_INCLUDED
#define CSS_SHEET_H_INCLUDED

#include "css/rule.h"
#include "css/style.h"
#include "css/value.h"

#include <map>
#include <string>

namespace css {

  class CompoundStyle;
  class Query;
  class StatefulStyle;

  class Sheet {
  public:
    Sheet();

    void addRule(Rule* rule);
    void addStyle(Style* style);

    Style* getStyle(const std::string& name);

    Query query(const StatefulStyle& stateful);
    CompoundStyle compoundStyle(const std::string& name);

  private:
    Rules m_rules;
    Styles m_styles;
  };

} // namespace css

#endif
