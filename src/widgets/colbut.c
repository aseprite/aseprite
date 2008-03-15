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

#include "jinete/jbutton.h"
#include "jinete/jdraw.h"
#include "jinete/jhook.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

#include "core/color.h"
#include "dialogs/colsel.h"
#include "modules/gfx.h"
#include "widgets/colbut.h"

typedef struct ColorButton
{
  color_t color;
  int imgtype;
} ColorButton;

static ColorButton *color_button_data(JWidget widget);
static bool color_button_msg_proc(JWidget widget, JMessage msg);
static void color_button_draw(JWidget widget);

JWidget color_button_new(color_t color, int imgtype)
{
  JWidget widget = jbutton_new("");
  ColorButton *color_button = jnew(ColorButton, 1);

  color_button->color = color;
  color_button->imgtype = imgtype;

  jwidget_add_hook(widget, color_button_type(),
		   color_button_msg_proc, color_button);
  jwidget_focusrest(widget, TRUE);

  return widget;
}

int color_button_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

int color_button_get_imgtype(JWidget widget)
{
  ColorButton *color_button = color_button_data(widget);

  return color_button->imgtype;
}

color_t color_button_get_color(JWidget widget)
{
  ColorButton *color_button = color_button_data(widget);

  return color_button->color;
}

void color_button_set_color(JWidget widget, color_t color)
{
  ColorButton *color_button = color_button_data(widget);

  color_button->color = color;
  jwidget_dirty(widget);
}

static ColorButton *color_button_data(JWidget widget)
{
  return jwidget_get_data(widget, color_button_type());
}

static bool color_button_msg_proc(JWidget widget, JMessage msg)
{
  ColorButton *color_button = color_button_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree(color_button);
      break;

    case JM_REQSIZE: {
      struct jrect box;

      jwidget_get_texticon_info(widget, &box, NULL, NULL, 0, 0, 0);

      box.x2 = box.x1+64;

      msg->reqsize.w = jrect_w(&box) + widget->border_width.l + widget->border_width.r;
      msg->reqsize.h = jrect_h(&box) + widget->border_width.t + widget->border_width.b;
      return TRUE;
    } 

    case JM_DRAW:
      color_button_draw(widget);
      return TRUE;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
	color_t color = color_button->color;

	if (ji_color_select(color_button->imgtype, &color)) {
	  color_button_set_color(widget, color);
	  jwidget_emit_signal(widget, SIGNAL_COLOR_BUTTON_CHANGE);
	}
	return TRUE;
      }
      break;
  }

  return FALSE;
}

static void color_button_draw(JWidget widget)
{
  ColorButton *color_button = color_button_data(widget);
  struct jrect box, text, icon;
  int x1, y1, x2, y2;
  int bg, c1, c2;
  char buf[256];

  jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);

  /* with mouse */
  if (jwidget_has_mouse(widget))
    bg = makecol(224, 224, 224);
  /* without mouse */
  else
    bg = makecol(192, 192, 192);

  /* selected */
  if (jwidget_is_selected(widget)) {
    c1 = makecol(128, 128, 128);
    c2 = makecol(255, 255, 255);
  }
  /* non-selected */
  else {
    c1 = makecol(255, 255, 255);
    c2 = makecol(128, 128, 128);
  }

  /* widget position */
  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  /* extern background */
  rectfill(ji_screen, x1, y1, x2, y2, makecol (192, 192, 192));

  /* 1st border */
  if (jwidget_has_focus(widget))
    bevel_box(ji_screen, x1, y1, x2, y2,
	      makecol(0, 0, 0), makecol(0, 0, 0), 2);
  else
    bevel_box(ji_screen, x1, y1, x2, y2, c1, c2, 1);

  /* 2nd border */
  x1++, y1++, x2--, y2--;
  if (jwidget_has_focus(widget))
    bevel_box(ji_screen, x1, y1, x2, y2, c1, c2, 1);
  else
    bevel_box(ji_screen, x1, y1, x2, y2, bg, bg, 0);

  /* background */
  x1++, y1++, x2--, y2--;
  draw_color(ji_screen, x1, y1, x2, y2,
	     color_button->imgtype,
	     color_button->color);

  /* draw text */
  color_to_formalstring(color_button->imgtype,
			color_button->color, buf, sizeof(buf), FALSE);

  jwidget_set_text_soft(widget, buf);
  jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);

  rectfill(ji_screen, text.x1, text.y1, text.x2-1, text.y2-1, makecol(0, 0, 0));
  jdraw_text(widget->text_font, widget->text, text.x1, text.y1,
	     makecol(255, 255, 255),
	     makecol(0, 0, 0), FALSE);
}
