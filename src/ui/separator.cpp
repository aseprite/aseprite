// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/separator.h"

#include "gfx/size.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"

using namespace gfx;

namespace ui {

Separator::Separator(const char* text, int align)
 : Widget(kSeparatorWidget)
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

  UI_FOREACH_WIDGET(getChildren(), it) {
    Widget* child = *it;
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
