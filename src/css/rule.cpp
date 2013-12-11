// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/rule.h"

namespace css {

Rule::Rule(const std::string& name) :
  m_name(name)
{
}

} // namespace css
