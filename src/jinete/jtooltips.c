/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008  David A. Capello
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

#include "jinete/jinete.h"

typedef struct TipData
{
  JWidget widget;		/* widget that shows the tooltip */
  JWidget window;		/* window where is the tooltip */
  char *text;
  int timer_id;
} TipData;

static int tip_type(void);
static bool tip_hook(JWidget widget, JMessage msg);

static JWidget tip_window_new(const char *text);
static bool tip_window_hook(JWidget widget, JMessage msg);

void jwidget_add_tooltip_text(JWidget widget, const char *text)
{
  TipData *tip = jnew(TipData, 1);

  tip->widget = widget;
  tip->window = NULL;
  tip->text = jstrdup(text);
  tip->timer_id = -1;

  jwidget_add_hook(widget, tip_type(), tip_hook, tip);
}

static int tip_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

/* hook for the widget in which we added a tooltip */
static bool tip_hook(JWidget widget, JMessage msg)
{
  TipData *tip = jwidget_get_data(widget, tip_type());

  switch (msg->type) {

    case JM_DESTROY:
      if (tip->timer_id >= 0)
	jmanager_remove_timer(tip->timer_id);

      jfree(tip->text);
      jfree(tip);
      break;

    case JM_MOUSEENTER:
      if (tip->timer_id < 0)
	tip->timer_id = jmanager_add_timer(widget, 300);

      jmanager_start_timer(tip->timer_id);
      break;

    case JM_CHAR:
    case JM_KEYPRESSED:
    case JM_BUTTONPRESSED:
    case JM_MOUSELEAVE:
      if (tip->window) {
	jwindow_close(tip->window, NULL);
	jwidget_free(tip->window);
	tip->window = NULL;
      }

      if (tip->timer_id >= 0)
	jmanager_stop_timer(tip->timer_id);
      break;

    case JM_TIMER:
      if (msg->timer.timer_id == tip->timer_id) {
	if (!tip->window) {
	  JWidget window = tip_window_new(tip->text);
	  int x = tip->widget->rc->x1;
	  int y = tip->widget->rc->y2;
	  int w = jrect_w(window->rc);
	  int h = jrect_h(window->rc);

	  tip->window = window;

	  jwindow_remap(window);
	  jwindow_position(window,
			   MID(0, x, JI_SCREEN_W-w),
			   MID(0, y, JI_SCREEN_H-h));
	  jwindow_open(window);
	}
	jmanager_stop_timer(tip->timer_id);
      }
      break;

  }
  return FALSE;
}

static JWidget tip_window_new(const char *text)
{
  JWidget window = jwindow_new(text);
  JLink link, next;

  jwindow_sizeable(window, FALSE);
  jwindow_moveable(window, FALSE);
  jwindow_wantfocus(window, FALSE);

/*   jwidget_set_align(window, JI_CENTER | JI_MIDDLE); */
  jwidget_set_align(window, JI_LEFT | JI_TOP);

  /* remove decorative widgets */
  JI_LIST_FOR_EACH_SAFE(window->children, link, next) {
    jwidget_free(link->data);
  }

  jwidget_add_hook(window, JI_WIDGET, tip_window_hook, NULL);
  jwidget_init_theme(window);

  return window;
}

static bool tip_window_hook(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      _ji_theme_textbox_draw(NULL, widget,
			     &msg->reqsize.w,
			     &msg->reqsize.h, 0, 0);

/*       msg->reqsize.w += widget->border_width.l + widget->border_width.r; */
/*       msg->reqsize.h += widget->border_width.t + widget->border_width.b; */
      return TRUE;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	widget->border_width.l = 3;
	widget->border_width.t = 3;
	widget->border_width.r = 3;
	widget->border_width.b = 3;
	return TRUE;
      }
      break;

    case JM_MOUSELEAVE:
    case JM_BUTTONPRESSED:
      jwindow_close(widget, NULL);
      break;

    case JM_DRAW: {
      JRect pos = jwidget_get_rect(widget);

      jdraw_rect(pos, makecol(0, 0, 0));

      jrect_shrink(pos, 1);
      jdraw_rectfill(pos, makecol(255, 255, 140));

      _ji_theme_textbox_draw(ji_screen, widget, NULL, NULL,
			     makecol(255, 255, 140),
			     makecol(0, 0, 0));
      return TRUE;
    }

  }
  return FALSE;
}
