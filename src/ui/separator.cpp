// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

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

Separator::Separator(const base::string& text, int align)
 : Widget(kSeparatorWidget)
{
  setAlign(align);
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

  if (hasText())
    maxSize.w = MAX(maxSize.w, jwidget_get_text_length(this));

  int w = this->border_width.l + maxSize.w + this->border_width.r;
  int h = this->border_width.t + maxSize.h + this->border_width.b;

  ev.setPreferredSize(Size(w, h));
}

} // namespace ui
