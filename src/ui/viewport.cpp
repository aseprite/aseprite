// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/resize_event.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/viewport.h"

namespace ui {

using namespace gfx;

Viewport::Viewport()
  : Widget(kViewViewportWidget)
{
  enableFlags(IGNORE_MOUSE);
  initTheme();
}

void Viewport::onResize(ResizeEvent& ev)
{
  Rect rect = ev.bounds();
  setBoundsQuietly(rect);

  Point scroll = static_cast<View*>(parent())->viewScroll();

  Rect cpos(0, 0, 0, 0);
  cpos.x = rect.x + border().left() - scroll.x;
  cpos.y = rect.y + border().top() - scroll.y;

  for (auto child : children()) {
    Size reqSize = child->sizeHint();

    cpos.w = MAX(reqSize.w, rect.w - border().width());
    cpos.h = MAX(reqSize.h, rect.h - border().height());

    child->setBounds(cpos);
  }
}

void Viewport::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(1 + border().width(),
                           1 + border().height()));
}

void Viewport::onPaint(PaintEvent& ev)
{
  theme()->paintViewViewport(ev);
}

Size Viewport::calculateNeededSize()
{
  Size maxSize(0, 0);
  Size reqSize;

  for (auto child : children()) {
    reqSize = child->sizeHint();
    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  return maxSize;
}

} // namespace ui
