// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gfx/point.h"
#include "gfx/size.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/viewport.h"

using namespace gfx;

namespace ui {

Viewport::Viewport()
  : Widget(JI_VIEW_VIEWPORT)
{
  initTheme();
}

bool Viewport::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      set_position(&msg->setpos.rect);
      return true;
  }

  return Widget::onProcessMessage(msg);
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

void Viewport::set_position(JRect rect)
{
  Size reqSize;
  JRect cpos;

  jrect_copy(this->rc, rect);

  Point scroll = static_cast<View*>(this->getParent())->getViewScroll();

  cpos = jrect_new(0, 0, 0, 0);
  cpos->x1 = this->rc->x1 + this->border_width.l - scroll.x;
  cpos->y1 = this->rc->y1 + this->border_width.t - scroll.y;

  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
    reqSize = child->getPreferredSize();

    cpos->x2 = cpos->x1 + MAX(reqSize.w, jrect_w(this->rc)
                                         - this->border_width.l
                                         - this->border_width.r);

    cpos->y2 = cpos->y1 + MAX(reqSize.h, jrect_h(this->rc)
                                         - this->border_width.t
                                         - this->border_width.b);

    jwidget_set_rect(child, cpos);
  }

  jrect_free(cpos);
}

} // namespace ui
