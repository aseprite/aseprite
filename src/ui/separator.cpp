// Aseprite UI Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/separator.h"

#include "gfx/size.h"
#include "ui/message.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"

namespace ui {

using namespace gfx;

Separator::Separator(const std::string& text, int align)
 : Widget(kSeparatorWidget)
{
  enableFlags(IGNORE_MOUSE);
  setAlign(align);
  if (!text.empty())
    setText(text);

  initTheme();
}

void Separator::onSizeHint(SizeHintEvent& ev)
{
  Size maxSize(0, 0);

  for (auto child : children()) {
    Size reqSize = child->sizeHint();
    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  if (hasText()) {
    maxSize.w = MAX(maxSize.w, textWidth());
    maxSize.h = MAX(maxSize.h, textHeight());
  }

  int w = maxSize.w + border().width();
  int h = maxSize.h + border().height();

  ev.setSizeHint(Size(w, h));
}

} // namespace ui
