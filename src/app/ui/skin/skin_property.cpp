// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/skin_property.h"

#include "ui/widget.h"

namespace app {
namespace skin {

const char* SkinProperty::Name = "SkinProperty";

SkinProperty::SkinProperty()
  : Property(Name)
{
  m_look = NormalLook;
  m_miniFont = false;
}

SkinProperty::~SkinProperty()
{
}

SkinPropertyPtr get_skin_property(ui::Widget* widget)
{
  auto skinProp =
    std::static_pointer_cast<SkinProperty>(widget->getProperty(SkinProperty::Name));
  if (!skinProp) {
    skinProp = std::make_shared<SkinProperty>();
    widget->setProperty(skinProp);
  }
  return skinProp;
}

} // namespace skin
} // namespace app
