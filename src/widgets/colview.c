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

#include <allegro.h>
#include <string.h>

#include "jinete/jdraw.h"
#include "jinete/jfont.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

#include "modules/gfx.h"
#include "widgets/colview.h"

typedef struct ColorViewer
{
  color_t color;
  int imgtype;
} ColorViewer;

static ColorViewer *colorviewer_data(JWidget widget);
static bool colorviewer_msg_proc(JWidget widget, JMessage msg);

JWidget colorviewer_new(color_t color, int imgtype)
{
  JWidget widget = jwidget_new(colorviewer_type());
  ColorViewer *colorviewer = jnew(ColorViewer, 1);

  colorviewer->color = color;
  colorviewer->imgtype = imgtype;

  jwidget_add_hook(widget, colorviewer_type(),
		   colorviewer_msg_proc, colorviewer);
  jwidget_focusrest(widget, TRUE);
  jwidget_set_border(widget, 2, 2, 2, 2);
  jwidget_set_align(widget, JI_CENTER | JI_MIDDLE);

  return widget;
}

int colorviewer_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

int colorviewer_get_imgtype(JWidget widget)
{
  ColorViewer *colorviewer = colorviewer_data(widget);

  return colorviewer->imgtype;
}

color_t colorviewer_get_color(JWidget widget)
{
  ColorViewer *colorviewer = colorviewer_data(widget);

  return colorviewer->color;
}

void colorviewer_set_color(JWidget widget, color_t color)
{
  ColorViewer *colorviewer = colorviewer_data(widget);

  colorviewer->color = color;
  jwidget_dirty(widget);
}

static ColorViewer *colorviewer_data(JWidget widget)
{
  return (ColorViewer *)jwidget_get_data(widget,
					 colorviewer_type());
}

static bool colorviewer_msg_proc(JWidget widget, JMessage msg)
{
  ColorViewer *colorviewer = colorviewer_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree(colorviewer);
      break;

    case JM_REQSIZE: {
      msg->reqsize.w = ji_font_text_len(widget->text_font, "255,255,255,255");
      msg->reqsize.h = jwidget_get_text_height(widget);

      msg->reqsize.w += widget->border_width.l + widget->border_width.r;
      msg->reqsize.h += widget->border_width.t + widget->border_width.b;
      return TRUE;
    }

    case JM_DRAW: {
      struct jrect box, text, icon;
      JRect rect = jwidget_get_rect(widget);
      char buf[256];

      jdraw_rectedge(rect,
		     makecol(128, 128, 128),
		     makecol(255, 255, 255));

      jrect_shrink(rect, 1);
      jdraw_rect(rect,
		 jwidget_has_focus(widget) ? makecol(0, 0, 0):
					     makecol(192, 192, 192));

      /* draw color background */
      jrect_shrink(rect, 1);
      draw_color(ji_screen, rect->x1, rect->y1, rect->x2-1, rect->y2-1,
		 colorviewer->imgtype,
		 colorviewer->color);

      /* draw text */
      color_to_formalstring(colorviewer->imgtype,
			    colorviewer->color, buf, sizeof(buf), FALSE);

      jwidget_set_text_soft(widget, buf);
      jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);

      jdraw_rectfill(&text, makecol(0, 0, 0));
      jdraw_text(widget->text_font, widget->text, text.x1, text.y1,
		 makecol(255, 255, 255), makecol(0, 0, 0), FALSE);

      jrect_free(rect);
      return TRUE;
    }

    case JM_BUTTONPRESSED:
      jwidget_emit_signal(widget, SIGNAL_COLORVIEWER_SELECT);
      return TRUE;

  }

  return FALSE;
}
