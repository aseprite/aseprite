// Aseprite UI Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
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

#include <algorithm>

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

bool ListItem::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kDoubleClickMessage:
      // Propagate the message to the parent.
      if (parent())
        return parent()->sendMessage(msg);
      else
        break;
      break;
  }
  return Widget::onProcessMessage(msg);
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

    maxSize.w = std::max(maxSize.w, reqSize.w);
    maxSize.h = std::max(maxSize.h, reqSize.h);
  }

  w = maxSize.w + border().width();
  h = maxSize.h + border().height();

  ev.setSizeHint(Size(w, h));
}

} // namespace ui
