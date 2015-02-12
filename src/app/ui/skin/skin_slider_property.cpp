// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/skin_slider_property.h"

namespace app {
namespace skin {

const char* SkinSliderProperty::Name = "SkinSliderProperty";

SkinSliderProperty::SkinSliderProperty(ISliderBgPainter* painter)
  : Property(Name)
  , m_painter(painter)
{
}

SkinSliderProperty::~SkinSliderProperty()
{
  delete m_painter;
}

ISliderBgPainter* SkinSliderProperty::getBgPainter() const
{
  return m_painter;
}

} // namespace skin
} // namespace app
