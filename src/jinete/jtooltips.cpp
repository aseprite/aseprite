/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008-2009  David Capello
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

#include <assert.h>
#include <allegro.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

typedef struct TipData
{
  JWidget widget;		/* widget that shows the tooltip */
  JWidget window;		/* window where is the tooltip */
  char *text;
  int timer_id;
} TipData;

typedef struct TipWindow
{
  bool close_on_buttonpressed;
  JRegion hot_region;
  bool filtering;
} TipWindow;

static int tip_type();
static bool tip_hook(JWidget widget, JMessage msg);

static JWidget tipwindow_new(const char *text, bool close_on_buttonpressed);
static int tipwindow_type();
static TipWindow *tipwindow_data(JWidget widget);
static bool tipwindow_msg_proc(JWidget widget, JMessage msg);

void jwidget_add_tooltip_text(JWidget widget, const char *text)
{
  TipData* tip = reinterpret_cast<TipData*>(jwidget_get_data(widget, tip_type()));

  assert(text != NULL);

  if (tip == NULL) {
    tip = jnew(TipData, 1);

    tip->widget = widget;
    tip->window = NULL;
    tip->text = jstrdup(text);
    tip->timer_id = -1;

    jwidget_add_hook(widget, tip_type(), tip_hook, tip);
  }
  else {
    if (tip->text != NULL)
      jfree(tip->text);

    tip->text = jstrdup(text);
  }
}

/**
 * Creates a window to show a tool-tip.
 */
JWidget jtooltip_window_new(const char *text)
{
  JWidget window = tipwindow_new(text, FALSE);

  return window;
}

/**
 * @param widget The tooltip window.
 * @param region The new hot-region. This pointer is holded by the @a widget.
 * So you can't destroy it after calling this routine.
 */
void jtooltip_window_set_hotregion(JWidget widget, JRegion region)
{
  TipWindow *tipwindow = tipwindow_data(widget);

  assert(region != NULL);

  if (tipwindow->hot_region != NULL)
    jregion_free(tipwindow->hot_region);

  if (!tipwindow->filtering) {
    tipwindow->filtering = TRUE;
    jmanager_add_msg_filter(JM_MOTION, widget);
    jmanager_add_msg_filter(JM_BUTTONPRESSED, widget);
    jmanager_add_msg_filter(JM_KEYPRESSED, widget);
  }
  tipwindow->hot_region = region;
}

/********************************************************************/
/* hook for widgets that want a tool-tip */

static int tip_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

/* hook for the widget in which we added a tooltip */
static bool tip_hook(JWidget widget, JMessage msg)
{
  TipData* tip = reinterpret_cast<TipData*>(jwidget_get_data(widget, tip_type()));

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
	  JWidget window = tipwindow_new(tip->text, TRUE);
/* 	  int x = tip->widget->rc->x1; */
/* 	  int y = tip->widget->rc->y2; */
	  int x = jmouse_x(0)+12;
	  int y = jmouse_y(0)+12;
	  int w, h;

	  tip->window = window;

	  jwindow_remap(window);

	  w = jrect_w(window->rc);
	  h = jrect_h(window->rc);

	  if (x+w > JI_SCREEN_W) {
	    x = jmouse_x(0) - w - 4;
	    y = jmouse_y(0);
	  }

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

/********************************************************************/
/* TipWindow */

static JWidget tipwindow_new(const char *text, bool close_on_buttonpressed)
{
  JWidget window = jwindow_new(text);
  JLink link, next;
  TipWindow *tipwindow = jnew(TipWindow, 1);

  tipwindow->close_on_buttonpressed = close_on_buttonpressed;
  tipwindow->hot_region = NULL;
  tipwindow->filtering = FALSE;

  jwindow_sizeable(window, FALSE);
  jwindow_moveable(window, FALSE);
  jwindow_wantfocus(window, FALSE);

  jwidget_set_align(window, JI_LEFT | JI_TOP);

  /* remove decorative widgets */
  JI_LIST_FOR_EACH_SAFE(window->children, link, next)
    jwidget_free(reinterpret_cast<JWidget>(link->data));

  jwidget_add_hook(window, tipwindow_type(),
		   tipwindow_msg_proc, tipwindow);
  jwidget_init_theme(window);

  return window;
}

static int tipwindow_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static TipWindow *tipwindow_data(JWidget widget)
{
  return (TipWindow *)jwidget_get_data(widget,
				       tipwindow_type());
}

static bool tipwindow_msg_proc(JWidget widget, JMessage msg)
{
  TipWindow *tipwindow = tipwindow_data(widget);

  switch (msg->type) {

    case JM_CLOSE:
      if (tipwindow->filtering) {
	tipwindow->filtering = FALSE;
	jmanager_remove_msg_filter(JM_MOTION, widget);
	jmanager_remove_msg_filter(JM_BUTTONPRESSED, widget);
	jmanager_remove_msg_filter(JM_KEYPRESSED, widget);
      }
      break;

    case JM_DESTROY:
      if (tipwindow->filtering) {
	tipwindow->filtering = FALSE;
	jmanager_remove_msg_filter(JM_MOTION, widget);
	jmanager_remove_msg_filter(JM_BUTTONPRESSED, widget);
	jmanager_remove_msg_filter(JM_KEYPRESSED, widget);
      }
      if (tipwindow->hot_region != NULL) {
	jregion_free(tipwindow->hot_region);
      }
      jfree(tipwindow);
      break;

    case JM_REQSIZE: {
      int w = 0, h = 0;

      _ji_theme_textbox_draw(NULL, widget, &w, &h, 0, 0);

      msg->reqsize.w = w;
      msg->reqsize.h = widget->border_width.t + widget->border_width.b;

      if (!jlist_empty(widget->children)) {
	int max_w, max_h;
	int req_w, req_h;
	JWidget child;
	JLink link;

	max_w = max_h = 0;
	JI_LIST_FOR_EACH(widget->children, link) {
	  child = (JWidget)link->data;

	  jwidget_request_size(child, &req_w, &req_h);

	  max_w = MAX(max_w, req_w);
	  max_h = MAX(max_h, req_h);
	}

	msg->reqsize.w = MAX(msg->reqsize.w,
			     widget->border_width.l + max_w + widget->border_width.r);
	msg->reqsize.h += max_h;
      }
      return TRUE;
    }

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	int w = 0, h = 0;

	widget->border_width.l = 3;
	widget->border_width.t = 3;
	widget->border_width.r = 3;
	widget->border_width.b = 3;

	_ji_theme_textbox_draw(NULL, widget, &w, &h, 0, 0);

	widget->border_width.t = h-3;

	/* setup the background color */
	jwidget_set_bg_color(widget, makecol(255, 255, 200));
	return TRUE;
      }
      break;

    case JM_MOUSELEAVE:
      if (tipwindow->hot_region == NULL)
	jwindow_close(widget, NULL);
      break;

    case JM_KEYPRESSED:
      if (tipwindow->filtering && msg->key.scancode < KEY_MODIFIERS)
	jwindow_close(widget, NULL);
      break;

    case JM_BUTTONPRESSED:
      /* if the user click outside the window, we have to close the
	 tooltip window */
      if (tipwindow->filtering) {
	JWidget picked = jwidget_pick(widget, msg->mouse.x, msg->mouse.y);
	if (!picked || jwidget_get_window(picked) != widget) {
	  jwindow_close(widget, NULL);
	}
      }

      /* this is used when the user click inside a small text
	 tooltip */
      if (tipwindow->close_on_buttonpressed)
	jwindow_close(widget, NULL);
      break;

    case JM_MOTION:
      if (tipwindow->hot_region != NULL &&
	  jmanager_get_capture() == NULL) {
	struct jrect box;

	/* if the mouse is outside the hot-region we have to close the window */
	if (!jregion_point_in(tipwindow->hot_region,
			      msg->mouse.x, msg->mouse.y, &box)) {
	  jwindow_close(widget, NULL);
	}
      }
      break;

    case JM_DRAW: {
      JRect pos = jwidget_get_rect(widget);
      int oldt;

      jdraw_rect(pos, makecol(0, 0, 0));

      jrect_shrink(pos, 1);
      jdraw_rectfill(pos, widget->bg_color());

      oldt = widget->border_width.t;
      widget->border_width.t = 3;
      _ji_theme_textbox_draw(ji_screen, widget, NULL, NULL,
			     widget->bg_color(),
			     ji_color_foreground());
      widget->border_width.t = oldt;

      jrect_free(pos);
      return TRUE;
    }

  }
  return FALSE;
}
