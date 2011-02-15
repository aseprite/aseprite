// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "gfx/size.h"
#include "gui/list.h"
#include "gui/message.h"
#include "gui/rect.h"
#include "gui/theme.h"
#include "gui/widget.h"

using namespace gfx;

static bool separator_msg_proc(JWidget widget, JMessage msg);

JWidget ji_separator_new(const char* text, int align)
{
  Widget* widget = new Widget(JI_SEPARATOR);

  jwidget_add_hook(widget, JI_SEPARATOR, separator_msg_proc, NULL);
  widget->setAlign(align);
  widget->setText(text);
  widget->initTheme();

  return widget;
}

static bool separator_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      Size maxSize(0, 0);
      Size reqSize;
      JWidget child;
      JLink link;

      JI_LIST_FOR_EACH(widget->children, link) {
	child = (JWidget)link->data;

	reqSize = child->getPreferredSize();
	maxSize.w = MAX(maxSize.w, reqSize.w);
	maxSize.h = MAX(maxSize.h, reqSize.h);
      }

      if (widget->hasText())
	maxSize.w = MAX(maxSize.w, jwidget_get_text_length(widget));

      msg->reqsize.w = widget->border_width.l + maxSize.w + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + maxSize.h + widget->border_width.b;
      return true;
    }

    case JM_DRAW:
      widget->getTheme()->draw_separator(widget, &msg->draw.rect);
      return true;
  }

  return false;
}
