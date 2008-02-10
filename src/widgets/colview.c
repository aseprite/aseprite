/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <string.h>

#include "jinete/jdraw.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

#include "modules/color.h"
#include "widgets/colview.h"

#endif

typedef struct ColorViewer
{
  int imgtype;
} ColorViewer;

static bool color_viewer_msg_proc (JWidget widget, JMessage msg);

JWidget color_viewer_new (const char *color, int imgtype)
{
  JWidget widget = jwidget_new (color_viewer_type ());
  ColorViewer *color_viewer = jnew (ColorViewer, 1);

  color_viewer->imgtype = imgtype;

  jwidget_add_hook (widget, color_viewer_type (),
		      color_viewer_msg_proc, color_viewer);
  jwidget_focusrest (widget, TRUE);
  jwidget_set_text (widget, color);
  jwidget_set_border (widget, 2, 2, 2, 2);
  jwidget_set_align (widget, JI_CENTER | JI_MIDDLE);

  return widget;
}

int color_viewer_type (void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type ();
  return type;
}

int color_viewer_get_imgtype (JWidget widget)
{
  return ((ColorViewer *)jwidget_get_data (widget,
					     color_viewer_type ()))->imgtype;
}

const char *color_viewer_get_color (JWidget widget)
{
  return jwidget_get_text (widget);
}

void color_viewer_set_color (JWidget widget, const char *color)
{
  jwidget_set_text (widget, color);
}

static bool color_viewer_msg_proc (JWidget widget, JMessage msg)
{
  int imgtype = (int)widget->user_data[0];

  switch (msg->type) {

    case JM_DESTROY:
      jfree (jwidget_get_data (widget, color_viewer_type ()));
      break;

    case JM_REQSIZE: {
      msg->reqsize.w = jwidget_get_text_length (widget);
      msg->reqsize.h = jwidget_get_text_height (widget);

      msg->reqsize.w += widget->border_width.l + widget->border_width.r;
      msg->reqsize.h += widget->border_width.t + widget->border_width.b;
      return TRUE;
    }

    case JM_DRAW: {
      struct jrect box, text, icon;
      JRect rect = jwidget_get_rect (widget);
      char *old_text;
      char buf[256];

      jdraw_rectedge (rect, makecol (128, 128, 128),
			      makecol (255, 255, 255));

      jrect_shrink (rect, 1);
      jdraw_rect (rect,
		    jwidget_has_focus (widget) ? makecol (0, 0, 0):
						   makecol (192, 192, 192));

      /* draw color background */
      jrect_shrink (rect, 1);
      draw_color (ji_screen, rect->x1, rect->y1, rect->x2-1, rect->y2-1,
		  imgtype, widget->text);

      /* draw text */
      color_to_formalstring (imgtype, widget->text, buf, sizeof (buf), FALSE);

      old_text = widget->text;
      widget->text = buf;

      jwidget_get_texticon_info (widget, &box, &text, &icon, 0, 0, 0);

      jdraw_rectfill (&text, makecol (0, 0, 0));
      jdraw_text (widget->text_font, widget->text, text.x1, text.y1,
		    makecol (255, 255, 255), makecol (0, 0, 0), FALSE);
      widget->text = old_text;

      jrect_free (rect);
      return TRUE;
    }

    case JM_BUTTONPRESSED:
      jwidget_emit_signal (widget, SIGNAL_COLOR_VIEWER_SELECT);
      return TRUE;
  }

  return FALSE;
}
