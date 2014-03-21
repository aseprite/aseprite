// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

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

ListItem::ListItem(const base::string& text)
  : Widget(kListItemWidget)
{
  setAlign(JI_LEFT | JI_MIDDLE);
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
  UI_FOREACH_WIDGET(getChildren(), it)
    (*it)->setBounds(crect);
}

void ListItem::onPreferredSize(PreferredSizeEvent& ev)
{
  int w = 0, h = 0;
  Size maxSize;

  if (hasText())
    maxSize = getTextSize();
  else
    maxSize.w = maxSize.h = 0;

  UI_FOREACH_WIDGET(getChildren(), it) {
    Size reqSize = (*it)->getPreferredSize();

    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  w = this->border_width.l + maxSize.w + this->border_width.r;
  h = this->border_width.t + maxSize.h + this->border_width.b;

  ev.setPreferredSize(Size(w, h));
}

} // namespace ui
