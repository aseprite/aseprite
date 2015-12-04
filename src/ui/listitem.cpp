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
#include "ui/size_hint_event.h"
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
  theme()->paintListItem(ev);
}

void ListItem::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());

  Rect crect = childrenBounds();
  for (auto child : children())
    child->setBounds(crect);
}

void ListItem::onSizeHint(SizeHintEvent& ev)
{
  int w = 0, h = 0;
  Size maxSize;

  if (hasText())
    maxSize = textSize();
  else
    maxSize.w = maxSize.h = 0;

  for (auto child : children()) {
    Size reqSize = child->sizeHint();

    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  w = maxSize.w + border().width();
  h = maxSize.h + border().height();

  ev.setSizeHint(Size(w, h));
}

} // namespace ui
