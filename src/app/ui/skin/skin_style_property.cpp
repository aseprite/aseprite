// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/skin_style_property.h"

namespace app {
namespace skin {

const char* SkinStyleProperty::Name = "SkinStyleProperty";

SkinStyleProperty::SkinStyleProperty(Style* style)
  : Property(Name)
  , m_style(style)
{
}

Style* SkinStyleProperty::getStyle() const
{
  return m_style;
}

} // namespace skin
} // namespace app
