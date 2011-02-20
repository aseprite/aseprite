// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gfx/point.h"
#include "gfx/size.h"
#include "gui/list.h"
#include "gui/message.h"
#include "gui/theme.h"
#include "gui/view.h"
#include "gui/viewport.h"

using namespace gfx;

Viewport::Viewport()
  : Widget(JI_VIEW_VIEWPORT)
{
  initTheme();
}

bool Viewport::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      msg->reqsize.w = this->border_width.l + 1 + this->border_width.r;
      msg->reqsize.h = this->border_width.t + 1 + this->border_width.b;
      return true;

    case JM_SETPOS:
      set_position(&msg->setpos.rect);
      return true;

    case JM_DRAW:
      getTheme()->draw_view_viewport(this, &msg->draw.rect);
      return true;
  }

  return Widget::onProcessMessage(msg);
}

Size Viewport::calculateNeededSize()
{
  Size maxSize(0, 0);
  Size reqSize;
  JLink link;

  JI_LIST_FOR_EACH(this->children, link) {
    reqSize = ((Widget*)link->data)->getPreferredSize();
    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  return maxSize;
}

void Viewport::set_position(JRect rect)
{
  Size reqSize;
  JWidget child;
  JRect cpos;
  JLink link;

  jrect_copy(this->rc, rect);

  Point scroll = static_cast<View*>(this->getParent())->getViewScroll();

  cpos = jrect_new(0, 0, 0, 0);
  cpos->x1 = this->rc->x1 + this->border_width.l - scroll.x;
  cpos->y1 = this->rc->y1 + this->border_width.t - scroll.y;

  JI_LIST_FOR_EACH(this->children, link) {
    child = (Widget*)link->data;
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
