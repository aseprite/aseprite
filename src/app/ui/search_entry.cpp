// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/search_entry.h"

#include "app/ui/skin/skin_theme.h"
#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

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
        - getBounds().getOrigin();

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
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  theme->paintEntry(ev);

  auto icon = theme->parts.iconSearch()->getBitmap(0);
  Rect bounds = getClientBounds();
  ev.getGraphics()->drawColoredRgbaSurface(
    icon, theme->colors.text(),
    bounds.x + border().left(),
    bounds.y + bounds.h/2 - icon->height()/2);

  if (!getText().empty()) {
    icon = theme->parts.iconClose()->getBitmap(0);
    ev.getGraphics()->drawColoredRgbaSurface(
      icon, theme->colors.text(),
      bounds.x + bounds.w - border().right() - childSpacing() - icon->width(),
      bounds.y + bounds.h/2 - icon->height()/2);
  }
}

void SearchEntry::onSizeHint(SizeHintEvent& ev)
{
  Entry::onSizeHint(ev);
  Size sz = ev.sizeHint();

  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  auto icon = theme->parts.iconSearch()->getBitmap(0);
  sz.h = MAX(sz.h, icon->height()+border().height());

  ev.setSizeHint(sz);
}

Rect SearchEntry::onGetEntryTextBounds() const
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  Rect bounds = Entry::onGetEntryTextBounds();
  auto icon1 = theme->parts.iconSearch()->getBitmap(0);
  auto icon2 = theme->parts.iconClose()->getBitmap(0);
  bounds.x += childSpacing() + icon1->width();
  bounds.w -= 2*childSpacing() + icon1->width() + icon2->width();
  return bounds;
}

Rect SearchEntry::getCloseIconBounds() const
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  Rect bounds = getClientBounds();
  auto icon = theme->parts.iconClose()->getBitmap(0);
  bounds.x += bounds.w - border().right() - childSpacing() - icon->width();
  bounds.y += bounds.h/2 - icon->height()/2;
  bounds.w = icon->width();
  bounds.h = icon->height();
  return bounds;
}

} // namespace app
