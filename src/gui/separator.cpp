// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/separator.h"

#include "gfx/size.h"
#include "gui/list.h"
#include "gui/message.h"
#include "gui/preferred_size_event.h"
#include "gui/theme.h"

using namespace gfx;

namespace ui {

Separator::Separator(const char* text, int align)
 : Widget(JI_SEPARATOR)
{
  setAlign(align);
  setText(text);
  initTheme();
}

bool Separator::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_DRAW:
      getTheme()->draw_separator(this, &msg->draw.rect);
      return true;
  }

  return Widget::onProcessMessage(msg);
}

void Separator::onPreferredSize(PreferredSizeEvent& ev)
{
  Size maxSize(0, 0);
  JLink link;

  JI_LIST_FOR_EACH(this->children, link) {
    Widget* child = (Widget*)link->data;

    Size reqSize = child->getPreferredSize();
    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  if (hasText())
    maxSize.w = MAX(maxSize.w, jwidget_get_text_length(this));

  int w = this->border_width.l + maxSize.w + this->border_width.r;
  int h = this->border_width.t + maxSize.h + this->border_width.b;

  ev.setPreferredSize(Size(w, h));
}

} // namespace ui
