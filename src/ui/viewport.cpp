// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/resize_event.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/viewport.h"

namespace ui {

using namespace gfx;

Viewport::Viewport()
  : Widget(kViewViewportWidget)
{
  initTheme();
}

void Viewport::onResize(ResizeEvent& ev)
{
  Rect rect = ev.getBounds();
  setBoundsQuietly(rect);

  Point scroll = static_cast<View*>(this->getParent())->getViewScroll();

  Rect cpos(0, 0, 0, 0);
  cpos.x = rect.x + this->border_width.l - scroll.x;
  cpos.y = rect.y + this->border_width.t - scroll.y;

  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
    Size reqSize = child->getPreferredSize();

    cpos.w = MAX(reqSize.w, rect.w
                            - this->border_width.l
                            - this->border_width.r);

    cpos.h = MAX(reqSize.h, rect.h
                            - this->border_width.t
                            - this->border_width.b);

    child->setBounds(cpos);
  }
}

void Viewport::onPreferredSize(PreferredSizeEvent& ev)
{
  ev.setPreferredSize(gfx::Size(this->border_width.l + 1 + this->border_width.r,
                                this->border_width.t + 1 + this->border_width.b));
}

void Viewport::onPaint(PaintEvent& ev)
{
  getTheme()->paintViewViewport(ev);
}

Size Viewport::calculateNeededSize()
{
  Size maxSize(0, 0);
  Size reqSize;

  UI_FOREACH_WIDGET(getChildren(), it) {
    reqSize = (*it)->getPreferredSize();
    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  return maxSize;
}

} // namespace ui
