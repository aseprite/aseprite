// Aseprite UI Library
// Copyright (C) 2018-2025  Igara Studio S.A.
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

#include <algorithm>

namespace ui {

using namespace gfx;

Separator::Separator(const std::string& text, int align) : Widget(kSeparatorWidget)
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
    maxSize.w = std::max(maxSize.w, reqSize.w);
    maxSize.h = std::max(maxSize.h, reqSize.h);
  }

  if (hasText()) {
    maxSize.w = std::max(maxSize.w, textWidth());
    maxSize.h = std::max(maxSize.h, textHeight());
  }

  gfx::Border border = this->border();
  gfx::Border styleBorder;
  if (style()) {
    styleBorder = style()->margin() + style()->border() + style()->padding();
    if (hasText()) {
      styleBorder.left(2 * styleBorder.left());
      styleBorder.right(2 * styleBorder.right());
    }
  }

  int w = maxSize.w + std::max(border.width(), styleBorder.width());
  int h = maxSize.h + std::max(border.height(), styleBorder.height());

  ev.setSizeHint(Size(w, h));
}

} // namespace ui
