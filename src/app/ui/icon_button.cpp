// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/icon_button.h"

#include "app/ui/skin/skin_theme.h"
#include "os/surface.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"

namespace app {

using namespace ui;
using namespace app::skin;

IconButton::IconButton(const SkinPartPtr& part)
  : Button("")
  , m_part(part)
{
  initTheme();
}

void IconButton::onInitTheme(InitThemeEvent& ev)
{
  Button::onInitTheme(ev);

  auto theme = SkinTheme::get(this);
  setBgColor(theme->colors.menuitemNormalFace());
}

void IconButton::onSizeHint(SizeHintEvent& ev)
{
  os::Surface* icon = m_part->bitmap(0);
  ev.setSizeHint(
    gfx::Size(icon->width(),
              icon->height()) + 4*guiscale());
}

void IconButton::onPaint(PaintEvent& ev)
{
  auto theme = SkinTheme::get(this);
  Graphics* g = ev.graphics();
  gfx::Color fg, bg;

  if (isSelected()) {
    fg = theme->colors.menuitemHighlightText();
    bg = theme->colors.menuitemHighlightFace();
  }
  else if (isEnabled() && hasMouse()) {
    fg = theme->colors.menuitemHotText();
    bg = theme->colors.menuitemHotFace();
  }
  else {
    fg = theme->colors.menuitemNormalText();
    bg = bgColor();
  }

  g->fillRect(bg, g->getClipBounds());

  gfx::Rect bounds = clientBounds();
  os::Surface* icon = m_part->bitmap(0);
  g->drawColoredRgbaSurface(
    icon, fg,
    bounds.x+bounds.w/2-icon->width()/2,
    bounds.y+bounds.h/2-icon->height()/2);
}

} // namespace app
