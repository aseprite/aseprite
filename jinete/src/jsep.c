/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include "jinete/list.h"
#include "jinete/message.h"
#include "jinete/rect.h"
#include "jinete/theme.h"
#include "jinete/widget.h"

static bool separator_msg_proc (JWidget widget, JMessage msg);

JWidget ji_separator_new (const char *text, int align)
{
  JWidget widget = jwidget_new (JI_SEPARATOR);

  jwidget_add_hook (widget, JI_SEPARATOR, separator_msg_proc, NULL);
  jwidget_set_align (widget, align);
  jwidget_set_text (widget, text);
  jwidget_init_theme (widget);

  return widget;
}

static bool separator_msg_proc (JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      int max_w, max_h;
      int req_w, req_h;
      JWidget child;
      JLink link;

      max_w = max_h = 0;
      JI_LIST_FOR_EACH(widget->children, link) {
	child = (JWidget)link->data;

	jwidget_request_size (child, &req_w, &req_h);
	max_w = MAX (max_w, req_w);
	max_h = MAX (max_h, req_h);
      }

      if (widget->text)
	max_w = MAX (max_w, jwidget_get_text_length (widget));

      msg->reqsize.w = widget->border_width.l + max_w + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + max_h + widget->border_width.b;
      return TRUE;
    }
  }

  return FALSE;
}
