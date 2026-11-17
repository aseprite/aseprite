// Aseprite
// Copyright (C) 2021-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/zoom_entry.h"

#include "app/modules/gui.h"
#include "base/scoped_value.h"
#include "fmt/format.h"
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

ZoomEntry::ZoomEntry() : IntEntry(0, render::Zoom::linearValues() - 1, this), m_locked(false)
{
  setSuffix("%");
  setMaxTextLength(7); // strlen("25600.0") == 7
  setup_mini_look(this);

  setZoom(render::Zoom(1, 1));
  initTheme();
}

void ZoomEntry::setZoom(const render::Zoom& zoom)
{
  if (m_locked)
    return;

  std::string newText = fmt::format("{:.1f}", zoom.scale() * 100.0);
  if (newText != text())
    setText(newText);
}

void ZoomEntry::onValueChange()
{
  base::ScopedValue lock(m_locked, true);
  IntEntry::onValueChange();

  render::Zoom zoom = render::Zoom::fromRealScale(std::strtod(text().c_str(), nullptr) / 100.0);
  ZoomChange(zoom);
}

std::string ZoomEntry::onGetTextFromValue(int value)
{
  render::Zoom zoom = render::Zoom::fromLinearScale(value);
  return fmt::format("{:.1f}", zoom.scale() * 100.0);
}

int ZoomEntry::onGetValueFromText(const std::string& text)
{
  double value = std::strtod(text.c_str(), nullptr);
  return render::Zoom::fromRealScale(value / 100.0).linearScale();
}

} // namespace app
