/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro/draw.h>
#include <allegro/gfx.h>

#include "jinete/draw.h"
#include "jinete/message.h"
#include "jinete/rect.h"
#include "jinete/system.h"
#include "jinete/theme.h"
#include "jinete/widget.h"

static bool image_msg_proc (JWidget widget, JMessage msg);

JWidget ji_image_new (BITMAP *bmp, int align)
{
  JWidget widget = jwidget_new (JI_IMAGE);

  jwidget_add_hook (widget, JI_IMAGE, image_msg_proc, bmp);
  jwidget_set_align (widget, align);

  return widget;
}

static bool image_msg_proc (JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE: {
      BITMAP *bmp = jwidget_get_data (widget, JI_IMAGE);
      struct jrect box, text, icon;

      jwidget_get_texticon_info (widget, &box, &text, &icon,
				   jwidget_get_align (widget),
				   bmp->w, bmp->h);

      msg->reqsize.w = widget->border_width.l + jrect_w(&box) + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + jrect_h(&box) + widget->border_width.b;
      return TRUE;
    }

    case JM_DRAW: {
      BITMAP *bmp = jwidget_get_data (widget, JI_IMAGE);
      struct jrect box, text, icon;

      jwidget_get_texticon_info (widget, &box, &text, &icon,
				   jwidget_get_align (widget),
				   bmp->w, bmp->h);

      jdraw_rectexclude(widget->rc, &icon,
			jwidget_get_bg_color(widget));

      blit (bmp, ji_screen, 0, 0,
	    icon.x1, icon.y1, jrect_w(&icon), jrect_h(&icon));
      return TRUE;
    }
  }

  return FALSE;
}
