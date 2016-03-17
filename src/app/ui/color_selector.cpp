// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_selector.h"

#include "ui/size_hint_event.h"
#include "ui/theme.h"

namespace app {

using namespace ui;

ColorSelector::ColorSelector()
  : Widget(kGenericWidget)
  , m_lockColor(false)
{
}

void ColorSelector::selectColor(const app::Color& color)
{
  if (m_lockColor)
    return;

  m_color = color;
  invalidate();
}

void ColorSelector::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(32*ui::guiscale(), 32*ui::guiscale()));
}

} // namespace app
