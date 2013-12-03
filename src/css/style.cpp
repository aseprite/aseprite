// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "css/style.h"

namespace css {
  
Style::Style(const std::string& name, Style* base) :
  m_name(name),
  m_base(base) {
}

} // namespace css
