// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/style.h"

namespace css {
  
Style::Style(const std::string& name, const Style* base) :
  m_name(name),
  m_base(base) {
}

} // namespace css
