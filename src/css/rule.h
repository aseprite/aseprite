// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef CSS_RULE_H_INCLUDED
#define CSS_RULE_H_INCLUDED

#include "css/map.h"

#include <map>
#include <string>

namespace css {

  class Rule {
  public:
    Rule() { }
    Rule(const std::string& name);

    const std::string& name() const { return m_name; }

  private:
    std::string m_name;
  };

  typedef Map<Rule*> Rules;

} // namespace css

#endif
