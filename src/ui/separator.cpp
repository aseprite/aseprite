// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/separator.h"

#include "gfx/size.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"

namespace ui {

using namespace gfx;

Separator::Separator(const std::string& text, int align)
 : Widget(kSeparatorWidget)
{
  setAlign(align);
  if (!text.empty())
    setText(text);

  initTheme();
}

void Separator::onPaint(PaintEvent& ev)
{
  getTheme()->paintSeparator(ev);
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

  if (hasText()) {
    maxSize.w = MAX(maxSize.w, getTextWidth());
    maxSize.h = MAX(maxSize.h, getTextHeight());
  }

  int w = maxSize.w + border().width();
  int h = maxSize.h + border().height();

  ev.setPreferredSize(Size(w, h));
}

} // namespace ui
