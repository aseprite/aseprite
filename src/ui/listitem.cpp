// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/listitem.h"

#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/resize_event.h"
#include "ui/theme.h"
#include "ui/view.h"

namespace ui {

using namespace gfx;

ListItem::ListItem(const std::string& text)
  : Widget(kListItemWidget)
{
  setDoubleBuffered(true);
  setAlign(LEFT | MIDDLE);
  setText(text);
  initTheme();
}

void ListItem::onPaint(PaintEvent& ev)
{
  getTheme()->paintListItem(ev);
}

void ListItem::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());

  Rect crect = getChildrenBounds();
  for (auto child : children())
    child->setBounds(crect);
}

void ListItem::onPreferredSize(PreferredSizeEvent& ev)
{
  int w = 0, h = 0;
  Size maxSize;

  if (hasText())
    maxSize = getTextSize();
  else
    maxSize.w = maxSize.h = 0;

  for (auto child : children()) {
    Size reqSize = child->getPreferredSize();

    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  w = maxSize.w + border().width();
  h = maxSize.h + border().height();

  ev.setPreferredSize(Size(w, h));
}

} // namespace ui
