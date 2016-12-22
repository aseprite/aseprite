// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/zoom_entry.h"

#include "app/modules/gui.h"
#include "base/scoped_value.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/popup_window.h"
#include "ui/slider.h"
#include "ui/system.h"
#include "ui/theme.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace app {

using namespace gfx;
using namespace ui;

ZoomEntry::ZoomEntry()
  : IntEntry(
    0, render::Zoom::fromLinearScale(render::Zoom::linearValues()-1).scale()*100, this)
  , m_locked(false)
{
  setSuffix("%");
  setup_mini_look(this);

  setZoom(render::Zoom(1, 1));
}

void ZoomEntry::setZoom(const render::Zoom& zoom)
{
  if (m_locked)
    return;

  setText(onGetTextFromValue(zoom.linearScale()));
}

void ZoomEntry::onValueChange()
{
  base::ScopedValue<bool> lock(m_locked, true, m_locked);
  IntEntry::onValueChange();

  render::Zoom zoom = render::Zoom::fromLinearScale(getValue());
  ZoomChange(zoom);
}

std::string ZoomEntry::onGetTextFromValue(int value)
{
  render::Zoom zoom = render::Zoom::fromLinearScale(value);

  char buf[256];
  std::sprintf(buf, "%.1f", zoom.scale() * 100.0);
  return buf;
}

int ZoomEntry::onGetValueFromText(const std::string& text)
{
  double value = std::strtod(text.c_str(), nullptr);
  return render::Zoom::fromScale(value / 100.0).linearScale();
}

} // namespace app
