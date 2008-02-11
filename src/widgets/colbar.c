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

#include "jinete/jintern.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jwidget.h"

#include "core/app.h"
#include "core/cfg.h"
#include "dialogs/colsel.h"
#include "dialogs/minipal.h"
#include "modules/color.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"
#include "widgets/statebar.h"

static bool color_bar_msg_proc(JWidget widget, JMessage msg);

static void update_status_bar(ColorBar *color_bar, int color, int msecs);
static void get_info(JWidget widget, int *beg, int *end);

JWidget color_bar_new(int align)
{
  JWidget widget = jwidget_new(color_bar_type());
  ColorBar *color_bar = jnew0(ColorBar, 1);

  color_bar->widget = widget;
  color_bar->ncolor = 16;
  color_bar->select[0] = -1;
  color_bar->select[1] = -1;

  jwidget_add_hook(widget, color_bar_type(),
		   color_bar_msg_proc, color_bar);
  jwidget_focusrest(widget, TRUE);
  jwidget_set_align(widget, align);

  widget->border_width.l = 2;
  widget->border_width.t = 2;
  widget->border_width.r = 2;
  widget->border_width.b = 2;

  /* return the box */
  return widget;
}

int color_bar_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

ColorBar *color_bar_data(JWidget widget)
{
  return jwidget_get_data(widget, color_bar_type());
}

void color_bar_set_size(JWidget widget, int size)
{
  ColorBar *color_bar = color_bar_data(widget);

  color_bar->ncolor = MID(1, size, COLOR_BAR_COLORS);

  jwidget_dirty(widget);
}

const char *color_bar_get_color(JWidget widget, int num)
{
  ColorBar *color_bar = color_bar_data(widget);

  return color_bar->color[color_bar->select[num]];
}

void color_bar_set_color(JWidget widget, int num, const char *color, bool find)
{
  ColorBar *color_bar = color_bar_data(widget);
  int other = num == 1 ? 0: 1;

  /* find if the color is in other place */

  if (find) {
    int c, beg, end;

    get_info(widget, &beg, &end);

    for (c=beg; c<=end; c++) {
      if (c != color_bar->select[other] &&
	  ustrcmp(color_bar->color[c], color) == 0) {
	/* change the selection to the color found */
	color_bar->select[num] = c;

	jwidget_dirty(widget);
	update_status_bar(color_bar, color_bar->select[num], 100);
	return;
      }
    }
  }

  /* if the color doesn't exist in the color-bar, change the selected */
  if (color_bar->color[color_bar->select[num]])
    jfree(color_bar->color[color_bar->select[num]]);

  color_bar->color[color_bar->select[num]] = jstrdup(color);

  jwidget_dirty(widget);
  update_status_bar(color_bar, color_bar->select[num], 100);
}

void color_bar_set_color_directly(JWidget widget, int index, const char *color)
{
  ColorBar *color_bar = color_bar_data(widget);

  if (color_bar->color[index])
    jfree(color_bar->color[index]);

  color_bar->color[index] = jstrdup(color);

  jwidget_dirty(widget);
}

static bool color_bar_msg_proc (JWidget widget, JMessage msg)
{
  ColorBar *color_bar = color_bar_data(widget);

  switch (msg->type) {

    case JM_OPEN: {
      int ncolor = get_config_int ("ColorBar", "NColors", color_bar->ncolor);
      char buf[256], def[256];
      int c, beg, end;

      color_bar->ncolor = MID(1, ncolor, COLOR_BAR_COLORS);

      get_info (widget, &beg, &end);

      /* fill color-bar with saved colors in the configuration file */
      for (c=0; c<COLOR_BAR_COLORS; c++) {
	usprintf(buf, "Color%03d", c);
	usprintf(def, "index{%d}", c);
	color_bar->color[c] = jstrdup(get_config_string("ColorBar",
							buf, def));
      }

      /* get selected colors */
      color_bar->select[0] = get_config_int("ColorBar", "FG", 15);
      color_bar->select[1] = get_config_int("ColorBar", "BG", 0);
      color_bar->select[0] = MID(beg, color_bar->select[0], end);
      color_bar->select[1] = MID(beg, color_bar->select[1], end);
      break;
    }

    case JM_DESTROY: {
      char buf[256];
      int c;

      set_config_int("ColorBar", "NColors", color_bar->ncolor);
      set_config_int("ColorBar", "FG", color_bar->select[0]);
      set_config_int("ColorBar", "BG", color_bar->select[1]);

      for (c=0; c<color_bar->ncolor; c++) {
	usprintf(buf, "Color%03d", c);
	set_config_string("ColorBar", buf, color_bar->color[c]);
	jfree(color_bar->color[c]);
      }
      for (; c<COLOR_BAR_COLORS; c++)
	jfree(color_bar->color[c]);

      jfree(color_bar);
      break;
    }

    case JM_REQSIZE:
      msg->reqsize.w = msg->reqsize.h = 16;
      return TRUE;

    case JM_DRAW: {
      int imgtype = app_get_current_image_type();
      int x1, y1, x2, y2;
      int c, beg, end;

      get_info(widget, &beg, &end);

      x1 = widget->rc->x1;
      y1 = widget->rc->y1;
      x2 = widget->rc->x2-1;
      y2 = widget->rc->y2-1;

      _ji_theme_rectedge(ji_screen, x1, y1, x2, y2,
			 makecol(128, 128, 128), makecol(255, 255, 255));

      x1++, y1++, x2--, y2--;
      rect(ji_screen, x1, y1, x2, y2, makecol(192, 192, 192));

      x1++, y1++, x2--, y2--;

      if (widget->align & JI_HORIZONTAL) {
	int r, g, b;
	int u1, u2;

	for (c=beg; c<=end; c++) {
	  u1 = x1 + (x2-x1+1)*(c-beg  )/(end-beg+1);
	  u2 = x1 + (x2-x1+1)*(c-beg+1)/(end-beg+1) - 1;

	  draw_color(ji_screen, u1, y1, u2, y2, imgtype, color_bar->color[c]);

	  /* a selected color */
	  if ((c == color_bar->select[0]) || (c == color_bar->select[1])) {
	    char *new_color;
	    int color;

	    new_color = color_from_image
	      (imgtype, get_color_for_image(imgtype, color_bar->color[c]));

	    r = color_get_red(imgtype, new_color);
	    g = color_get_green(imgtype, new_color);
	    b = color_get_blue(imgtype, new_color);

	    jfree(new_color);

	    color = blackandwhite_neg(r, g, b);

	    if (c == color_bar->select[0])
	      rectfill(ji_screen, u1, y1, u2, y1+1, color);

	    if (c == color_bar->select[1])
	      rectfill(ji_screen, u1, y2-1, u2, y2, color);
	  }
	}
      }
      else {
	int r, g, b;
	int v1, v2;

	for (c=beg; c<=end; c++) {
	  v1 = y1 + (y2-y1+1)*(c-beg  )/(end-beg+1);
	  v2 = y1 + (y2-y1+1)*(c-beg+1)/(end-beg+1) - 1;

	  draw_color(ji_screen, x1, v1, x2, v2, imgtype, color_bar->color[c]);

	  /* a selected color */
	  if ((c == color_bar->select[0]) || (c == color_bar->select[1])) {
	    char *new_color;
	    int color;

	    new_color = color_from_image
	      (imgtype, get_color_for_image(imgtype, color_bar->color[c]));

	    r = color_get_red(imgtype, new_color);
	    g = color_get_green(imgtype, new_color);
	    b = color_get_blue(imgtype, new_color);

	    jfree(new_color);

	    color = blackandwhite_neg(r, g, b);

	    if (c == color_bar->select[0])
	      rectfill(ji_screen, x1, v1, x1+1, v2, color);

	    if (c == color_bar->select[1])
	      rectfill(ji_screen, x2-1, v1, x2, v2, color);
	  }
	}
      }
      return TRUE;
    }

    case JM_BUTTONPRESSED:
      jwidget_hard_capture_mouse(widget);

    case JM_MOTION: {
      int x1, y1, x2, y2;
      int c, beg, end;

      get_info(widget, &beg, &end);

      x1 = widget->rc->x1;
      y1 = widget->rc->y1;
      x2 = widget->rc->x2-1;
      y2 = widget->rc->y2-1;

      x1++, y1++, x2--, y2--;
      x1++, y1++, x2--, y2--;

      if (widget->align & JI_HORIZONTAL) {
	int u1, u2;

	for (c=beg; c<=end; c++) {
	  u1 = x1 + (x2-x1+1)*(c-beg  )/(end-beg+1);
	  u2 = x1 + (x2-x1+1)*(c-beg+1)/(end-beg+1) - 1;

	  if ((msg->mouse.x >= u1) && (msg->mouse.x <= u2)) {
	    /* change the color */
	    if ((jwidget_has_capture(widget)) ||
		(msg->type == JM_BUTTONPRESSED)) {
	      if (msg->mouse.left) {
		color_bar->select[0] = c;
		jwidget_dirty(widget);
	      }
	      else if (msg->mouse.right) {
		color_bar->select[1] = c;
		jwidget_dirty(widget);
	      }
	    }

	    /* change the text of the status bar */
	    update_status_bar(color_bar, c, 0);
	    break;
	  }
	}
      }
      else {
	int v1, v2;

	for (c=beg; c<=end; c++) {
	  v1 = y1 + (y2-y1+1)*(c-beg  )/(end-beg+1);
	  v2 = y1 + (y2-y1+1)*(c-beg+1)/(end-beg+1) - 1;

	  if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	    /* change the color */
	    if ((jwidget_has_capture(widget)) ||
		(msg->type == JM_BUTTONPRESSED)) {
	      int num = -1;

	      if (msg->mouse.left) {
		color_bar->select[num = 0] = c;
		jwidget_dirty(widget);
	      }
	      else if (msg->mouse.right) {
		color_bar->select[num = 1] = c;
		jwidget_dirty(widget);
	      }

	      /* show a minipal? */
	      if (msg->type == JM_BUTTONPRESSED &&
		  msg->any.shifts & KB_CTRL_FLAG && num >= 0) {
		ji_minipal_new(widget, x2, v1);
	      }
	    }

	    /* change the text of the status bar */
	    update_status_bar(color_bar, c, 0);
	    break;
	  }
	}
      }
      break;
    }

    case JM_BUTTONRELEASED:
      jwidget_release_mouse(widget);
      break;

    case JM_DOUBLECLICK: {
      int num = (msg->mouse.left) ? 0: 1;
      const char *src;
      char *dst;

      while (jmouse_b(0))
	jmouse_poll();

      /* get the color from the table */
      src = color_bar_get_color(widget, num);

      /* change this color with the color-select dialog */
      dst = ji_color_select(app_get_current_image_type(), src);

      /* set the color of the table */
      if (dst) {
	color_bar_set_color(widget, num, dst, FALSE);
	jfree(dst);
      }
      return TRUE;
    }
  }

  return FALSE;
}

static void update_status_bar(ColorBar *color_bar, int color, int msecs)
{
  char buf[128];

  color_to_formalstring(app_get_current_image_type(),
			color_bar->color[color], buf, sizeof(buf), TRUE);

  status_bar_set_text(app_get_status_bar(), msecs, "%s %s", _("Color"), buf);
}

static void get_info(JWidget widget, int *_beg, int *_end)
{
  ColorBar *color_bar = color_bar_data(widget);
  int beg, end;

  beg = 0;
  end = color_bar->ncolor-1;

  if (_beg) *_beg = beg;
  if (_end) *_end = end;
}
