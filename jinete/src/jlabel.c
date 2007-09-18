/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include "jinete/message.h"
#include "jinete/theme.h"
#include "jinete/widget.h"

static bool label_msg_proc (JWidget widget, JMessage msg);

JWidget jlabel_new (const char *text)
{
  JWidget widget = jwidget_new (JI_LABEL);

  jwidget_add_hook (widget, JI_LABEL, label_msg_proc, NULL);
  jwidget_set_align (widget, JI_LEFT | JI_MIDDLE);
  jwidget_set_text (widget, text);
  jwidget_init_theme (widget);

  return widget;
}

static bool label_msg_proc (JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      if (widget->text) {
	msg->reqsize.w = jwidget_get_text_length (widget);
	msg->reqsize.h = jwidget_get_text_height (widget);
      }
      else
	msg->reqsize.w = msg->reqsize.h = 0;

      msg->reqsize.w += widget->border_width.l + widget->border_width.r;
      msg->reqsize.h += widget->border_width.t + widget->border_width.b;
      return TRUE;
  }

  return FALSE;
}
