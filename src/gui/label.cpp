// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gui/message.h"
#include "gui/label.h"
#include "gui/theme.h"

Label::Label(const char *text)
  : Widget(JI_LABEL)
{
  this->setAlign(JI_LEFT | JI_MIDDLE);
  this->setText(text);
  initTheme();
}

bool Label::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      if (this->hasText()) {
	msg->reqsize.w = jwidget_get_text_length(this);
	msg->reqsize.h = jwidget_get_text_height(this);
      }
      else
	msg->reqsize.w = msg->reqsize.h = 0;

      msg->reqsize.w += this->border_width.l + this->border_width.r;
      msg->reqsize.h += this->border_width.t + this->border_width.b;
      return true;

  }

  return Widget::onProcessMessage(msg);
}

void Label::onPaint(PaintEvent& ev)
{
  getTheme()->paintLabel(ev);
}
