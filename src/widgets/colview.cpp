/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "gui/jdraw.h"
#include "gui/jfont.h"
#include "gui/jmessage.h"
#include "gui/jrect.h"
#include "gui/jsystem.h"
#include "gui/jtheme.h"
#include "gui/widget.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "widgets/colview.h"

using namespace gfx;

typedef struct ColorViewer
{
  Color color;
  int imgtype;
} ColorViewer;

static ColorViewer *colorviewer_data(JWidget widget);
static bool colorviewer_msg_proc(JWidget widget, JMessage msg);

JWidget colorviewer_new(const Color& color, int imgtype)
{
  Widget* widget = new Widget(colorviewer_type());
  ColorViewer *colorviewer = jnew(ColorViewer, 1);

  colorviewer->color = color;
  colorviewer->imgtype = imgtype;

  jwidget_add_hook(widget, colorviewer_type(),
		   colorviewer_msg_proc, colorviewer);
  jwidget_focusrest(widget, true);
  jwidget_set_border(widget, 2 * jguiscale());
  widget->setAlign(JI_CENTER | JI_MIDDLE);

  return widget;
}

int colorviewer_type()
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

Color colorviewer_get_color(JWidget widget)
{
  ColorViewer *colorviewer = colorviewer_data(widget);

  return colorviewer->color;
}

void colorviewer_set_color(JWidget widget, const Color& color)
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
      msg->reqsize.w = ji_font_text_len(widget->getFont(), "XXX 255,255,255 (255)");
      msg->reqsize.h = jwidget_get_text_height(widget);

      msg->reqsize.w += widget->border_width.l + widget->border_width.r;
      msg->reqsize.h += widget->border_width.t + widget->border_width.b;
      return true;
    }

    case JM_DRAW: {
      struct jrect box, text, icon;
      JRect rect = jwidget_get_rect(widget);

      jdraw_rectedge(rect,
		     makecol(128, 128, 128),
		     makecol(255, 255, 255));

      jrect_shrink(rect, 1);
      jdraw_rect(rect,
		 widget->hasFocus() ? makecol(0, 0, 0):
				      makecol(192, 192, 192));

      /* draw color background */
      jrect_shrink(rect, 1);
      draw_color(ji_screen, Rect(rect->x1, rect->y1, jrect_w(rect), jrect_h(rect)),
		 colorviewer->imgtype,
		 colorviewer->color);

      // Draw text
      std::string str =
	colorviewer->color.toFormalString(colorviewer->imgtype, false);

      widget->setTextQuiet(str.c_str());
      jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);

      jdraw_rectfill(&text, makecol(0, 0, 0));
      jdraw_text(ji_screen, widget->getFont(), widget->getText(), text.x1, text.y1,
		 makecol(255, 255, 255), makecol(0, 0, 0), false, jguiscale());

      jrect_free(rect);
      return true;
    }

    case JM_BUTTONPRESSED:
      jwidget_emit_signal(widget, SIGNAL_COLORVIEWER_SELECT);
      return true;

  }

  return false;
}
