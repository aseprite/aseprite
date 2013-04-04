// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/listitem.h"

#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"
#include "ui/view.h"

using namespace gfx;

namespace ui {

ListItem::ListItem(const char* text)
  : Widget(kListItemWidget)
{
  setAlign(JI_LEFT | JI_MIDDLE);
  setText(text);
  initTheme();
}

bool ListItem::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_SETPOS: {
      JRect crect;

      jrect_copy(this->rc, &msg->setpos.rect);
      crect = jwidget_get_child_rect(this);

      UI_FOREACH_WIDGET(getChildren(), it)
        jwidget_set_rect(*it, crect);

      jrect_free(crect);
      return true;
    }

    case JM_DRAW:
      this->getTheme()->draw_listitem(this, &msg->draw.rect);
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void ListItem::onPreferredSize(PreferredSizeEvent& ev)
{
  int w = 0, h = 0;
  Size maxSize;

  if (hasText()) {
    maxSize.w = jwidget_get_text_length(this);
    maxSize.h = jwidget_get_text_height(this);
  }
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
