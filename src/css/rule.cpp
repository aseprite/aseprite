// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

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
