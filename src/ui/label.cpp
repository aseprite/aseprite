// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/font.h"
#include "ui/label.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"

namespace ui {

Label::Label(const std::string& text)
  : Widget(kLabelWidget)
{
  setAlign(LEFT | MIDDLE);
  setText(text);
  initTheme();
}

void Label::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Size sz(0, 0);

  if (hasText()) {
    // Labels are not UIString
    sz.w = getFont()->textLength(getText().c_str());
    sz.h = getTextHeight();
  }

  sz.w += this->border_width.l + this->border_width.r;
  sz.h += this->border_width.t + this->border_width.b;

  ev.setPreferredSize(sz);
}

void Label::onPaint(PaintEvent& ev)
{
  getTheme()->paintLabel(ev);
}

} // namespace ui
