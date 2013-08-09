// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/label.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/theme.h"

namespace ui {

Label::Label(const char *text)
  : Widget(kLabelWidget)
{
  setAlign(JI_LEFT | JI_MIDDLE);
  setText(text);
  initTheme();
}

Color Label::getTextColor() const
{
  return m_textColor;
}

void Label::setTextColor(Color color)
{
  m_textColor = color;
}

void Label::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Size sz(0, 0);

  if (this->hasText()) {
    sz.w = jwidget_get_text_length(this);
    sz.h = jwidget_get_text_height(this);
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
