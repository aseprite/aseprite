// Aseprite
// Copyright (c) 2020  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/search_entry.h"

#include "app/ui/skin/skin_theme.h"
#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

#include <algorithm>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

SearchEntry::SearchEntry()
  : Entry(256, "")
{
}

bool SearchEntry::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage: {
      Rect closeBounds = getCloseIconBounds();
      Point mousePos = static_cast<MouseMessage*>(msg)->position()
        - bounds().origin();

      if (closeBounds.contains(mousePos)) {
        setText("");
        onChange();
        return true;
      }
      break;
    }
  }
  return Entry::onProcessMessage(msg);
}

void SearchEntry::onPaint(ui::PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  theme->paintEntry(ev);

  auto icon = theme->parts.iconSearch()->bitmap(0);
  Rect bounds = clientBounds();
  ev.graphics()->drawColoredRgbaSurface(
    icon, theme->colors.text(),
    bounds.x + border().left(),
    bounds.y + bounds.h/2 - icon->height()/2);

  if (!text().empty()) {
    icon = theme->parts.iconClose()->bitmap(0);
    ev.graphics()->drawColoredRgbaSurface(
      icon, theme->colors.text(),
      bounds.x + bounds.w - border().right() - childSpacing() - icon->width(),
      bounds.y + bounds.h/2 - icon->height()/2);
  }
}

void SearchEntry::onSizeHint(SizeHintEvent& ev)
{
  Entry::onSizeHint(ev);
  Size sz = ev.sizeHint();

  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  auto icon = theme->parts.iconSearch()->bitmap(0);
  sz.h = std::max(sz.h, icon->height()+border().height());

  ev.setSizeHint(sz);
}

Rect SearchEntry::onGetEntryTextBounds() const
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  Rect bounds = Entry::onGetEntryTextBounds();
  auto icon1 = theme->parts.iconSearch()->bitmap(0);
  auto icon2 = theme->parts.iconClose()->bitmap(0);
  bounds.x += childSpacing() + icon1->width();
  bounds.w -= 2*childSpacing() + icon1->width() + icon2->width();
  return bounds;
}

Rect SearchEntry::getCloseIconBounds() const
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  Rect bounds = clientBounds();
  auto icon = theme->parts.iconClose()->bitmap(0);
  bounds.x += bounds.w - border().right() - childSpacing() - icon->width();
  bounds.y += bounds.h/2 - icon->height()/2;
  bounds.w = icon->width();
  bounds.h = icon->height();
  return bounds;
}

} // namespace app
