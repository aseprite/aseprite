/* Jinete - a GUI library
 * Copyright (C) 2003-2008 David A. Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the Jinete nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define REDRAW_MOVEMENT
#define MOTION_CURSOR JI_CURSOR_NORMAL

#include <allegro.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

typedef struct Window
{
  JWidget killer;
  bool is_desktop : 1;
  bool is_moveable : 1;
  bool is_sizeable : 1;
  bool is_ontop : 1;
  bool is_wantfocus : 1;
  bool is_foreground : 1;
  bool is_autoremap : 1;
} Window;

enum {
  WINDOW_NONE = 0,
  WINDOW_MOVE = 1,
  WINDOW_RESIZE_LEFT = 2,
  WINDOW_RESIZE_RIGHT = 4,
  WINDOW_RESIZE_TOP = 8,
  WINDOW_RESIZE_BOTTOM = 16,
};

static JRect click_pos = NULL;
static int press_x, press_y;
static int window_action = WINDOW_NONE;

static JWidget window_new(int desktop, const char *text);
static bool window_msg_proc(JWidget widget, JMessage msg);
static void window_request_size(JWidget widget, int *w, int *h);
static void window_set_position(JWidget widget, JRect rect);

static int get_action(JWidget widget, int x, int y);
static void limit_size(JWidget widget, int *w, int *h);
static void move_window(JWidget widget, JRect rect, bool use_blit);
static void displace_widgets(JWidget widget, int x, int y);

bool _jwindow_is_moving(void)
{
  return (window_action == WINDOW_MOVE) ? TRUE: FALSE;
}

JWidget jwindow_new(const char *text)
{
  return window_new(FALSE, text);
}

JWidget jwindow_new_desktop(void)
{
  return window_new(TRUE, NULL);
}

JWidget jwindow_get_killer(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW);

  return window->killer;
}

void jwindow_moveable(JWidget widget, bool state)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 

  window->is_moveable = state;
}

void jwindow_sizeable(JWidget widget, bool state)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 

  window->is_sizeable = state;
}

void jwindow_ontop(JWidget widget, bool state)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 

  window->is_ontop = state;
}

void jwindow_wantfocus(JWidget widget, bool state)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 

  window->is_wantfocus = state;
}

void jwindow_remap(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 
  int req_w, req_h;
  JRect rect;

  if (window->is_autoremap) {
    window->is_autoremap = FALSE;
    jwidget_show(widget);
  }

  jwidget_request_size(widget, &req_w, &req_h);

  rect = jrect_new(widget->rc->x1, widget->rc->y1,
		   widget->rc->x1+req_w,
		   widget->rc->y1+req_h);
  jwidget_set_rect(widget, rect);
  jrect_free(rect);

  jwidget_emit_signal(widget, JI_SIGNAL_WINDOW_RESIZE);
  jwidget_dirty(widget);
}

void jwindow_center(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 
  JWidget manager = jwidget_get_manager(widget);

  if (window->is_autoremap)
    jwindow_remap(widget);

  jwindow_position(widget,
		   jrect_w(manager->rc)/2 - jrect_w(widget->rc)/2,
		   jrect_h(manager->rc)/2 - jrect_h(widget->rc)/2);
}

void jwindow_position(JWidget widget, int x, int y)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 
  int old_action = window_action;
  JRect rect;

  window_action = WINDOW_MOVE;

  if (window->is_autoremap)
    jwindow_remap(widget);

  rect = jrect_new(x, y, x+jrect_w(widget->rc), y+jrect_h(widget->rc));
  jwidget_set_rect(widget, rect);
  jrect_free(rect);

  window_action = old_action;

  jwidget_dirty(widget);
}

void jwindow_move(JWidget widget, JRect rect)
{
  move_window(widget, rect, TRUE);
}

void jwindow_open(JWidget widget)
{
  if (!widget->parent) {
    Window *window = jwidget_get_data(widget, JI_WINDOW); 

    if (window->is_autoremap)
      jwindow_center(widget);

    _jmanager_open_window(ji_get_default_manager(), widget);
  }
}

void jwindow_open_fg(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 
  JWidget manager;

  jwindow_open(widget);
  manager = jwidget_get_manager(widget);

  window->is_foreground = TRUE;

  while (!(widget->flags & JI_HIDDEN)) {
    if (jmanager_generate_messages(manager))
      jmanager_dispatch_messages(manager);
  }

  window->is_foreground = FALSE;
}

void jwindow_open_bg(JWidget widget)
{
  jwindow_open(widget);
}

void jwindow_close(JWidget widget, JWidget killer)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW);

  window->killer = killer;

  _jmanager_close_window(jwidget_get_manager(widget), widget, TRUE);
}

bool jwindow_is_toplevel(JWidget widget)
{
  JWidget manager = jwidget_get_manager(widget);

  if (!jlist_empty(manager->children))
    return (widget == jlist_first(manager->children)->data);
  else
    return FALSE;
}

bool jwindow_is_foreground(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 

  return window->is_foreground;
}

bool jwindow_is_desktop(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW);

  return window->is_desktop;
}

bool jwindow_is_ontop(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 

  return window->is_ontop;
}

bool jwindow_is_wantfocus(JWidget widget)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW); 

  return window->is_wantfocus;
}

static JWidget window_new(int desktop, const char *text)
{
  JWidget widget = jwidget_new(JI_WINDOW);
  Window *window = jnew(Window, 1);

  window->killer = NULL;
  window->is_desktop = desktop;
  window->is_moveable = !desktop;
  window->is_sizeable = !desktop;
  window->is_ontop = FALSE;
  window->is_wantfocus = TRUE;
  window->is_foreground = FALSE;
  window->is_autoremap = TRUE;

  jwidget_hide(widget);
  jwidget_add_hook(widget, JI_WINDOW, window_msg_proc, window);
  jwidget_set_text(widget, text);
  jwidget_set_align(widget, JI_LEFT | JI_MIDDLE);

  jwidget_init_theme(widget);

  return widget;
}

static bool window_msg_proc(JWidget widget, JMessage msg)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW);

  switch (msg->type) {

    case JM_DESTROY:
      _jmanager_close_window(jwidget_get_manager(widget), widget, FALSE);
      jfree(window);
      break;

    case JM_REQSIZE:
      window_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_SETPOS:
      window_set_position(widget, &msg->setpos.rect);
      return TRUE;

    case JM_OPEN:
      window->killer = NULL;
      break;

    case JM_CLOSE:
      jwidget_emit_signal(widget, JI_SIGNAL_WINDOW_CLOSE);
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_SET_TEXT)
	jwidget_init_theme(widget);
      break;

    case JM_BUTTONPRESSED: {
      if (!window->is_moveable)
	break;

      if (click_pos == NULL)
	click_pos = jrect_new_copy(widget->rc);
      else
	jrect_copy(click_pos, widget->rc);

      press_x = msg->mouse.x;
      press_y = msg->mouse.y;
      window_action = get_action(widget, press_x, press_y);
      if (window_action != WINDOW_NONE) {
	jwidget_hard_capture_mouse(widget);
	return TRUE;
      }
      else
	break;
    }

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(widget)) {
	jwidget_release_mouse(widget);
	jmouse_set_cursor(JI_CURSOR_NORMAL);

	if (click_pos != NULL) {
	  jrect_free(click_pos);
	  click_pos = NULL;
	}

	window_action = WINDOW_NONE;
	return TRUE;
      }
      break;

    case JM_MOTION:
      if (!window->is_moveable)
	break;

      /* does it have the mouse captured? */
      if (jwidget_has_capture(widget)) {
	/* reposition/resize */
	if (window_action == WINDOW_MOVE) {
	  int x = click_pos->x1 + (msg->mouse.x - press_x);
	  int y = click_pos->y1 + (msg->mouse.y - press_y);
	  JRect rect = jrect_new(x, y,
				 x+jrect_w(widget->rc),
				 y+jrect_h(widget->rc));
	  move_window(widget, rect, TRUE);
	  jrect_free(rect);
	}
	else {
	  int x, y, w, h;

	  w = jrect_w(click_pos);
	  h = jrect_h(click_pos);

	  if (window_action & WINDOW_RESIZE_LEFT)
	    w += press_x - msg->mouse.x;
	  else if (window_action & WINDOW_RESIZE_RIGHT)
	    w += msg->mouse.x - press_x;

	  if (window_action & WINDOW_RESIZE_TOP)
	    h += (press_y - msg->mouse.y);
	  else if (window_action & WINDOW_RESIZE_BOTTOM)
	    h += (msg->mouse.y - press_y);

	  limit_size(widget, &w, &h);

	  if ((jrect_w(widget->rc) != w) ||
	      (jrect_h(widget->rc) != h)) {
	    if (window_action & WINDOW_RESIZE_LEFT)
	      x = click_pos->x1 - (w - jrect_w(click_pos));
	    else
	      x = widget->rc->x1;

	    if (window_action & WINDOW_RESIZE_TOP)
	      y = click_pos->y1 - (h - jrect_h(click_pos));
	    else
	      y = widget->rc->y1;

	    {
	      JRect rect = jrect_new(x, y, x+w, y+h); 
	      move_window(widget, rect, FALSE);
	      jrect_free(rect);

	      jwidget_emit_signal(widget, JI_SIGNAL_WINDOW_RESIZE);
	      jwidget_dirty(widget);
	    }
	  }
	}
      }

      /* TODO */
/*       { */
/* 	JWidget manager = jwindow_get_manager(widget); */
/* 	JWidget view = jwidget_get_view(manager); */
/* 	if (view) { */
/* 	  jview_update(view); */
/* 	} */
/*       } */
      break;

    case JM_SETCURSOR:
      if (window->is_moveable) {
	int action = get_action(widget,
				msg->mouse.x,
				msg->mouse.y);
	int cursor = JI_CURSOR_NORMAL;

	if (action == WINDOW_MOVE)
	  cursor = MOTION_CURSOR;
	else if (action & WINDOW_RESIZE_LEFT) {
	  if (action & WINDOW_RESIZE_TOP)
	    cursor = JI_CURSOR_SIZE_TL;
	  else if (action & WINDOW_RESIZE_BOTTOM)
	    cursor = JI_CURSOR_SIZE_BL;
	  else
	    cursor = JI_CURSOR_SIZE_L;
	}
	else if (action & WINDOW_RESIZE_RIGHT) {
	  if (action & WINDOW_RESIZE_TOP)
	    cursor = JI_CURSOR_SIZE_TR;
	  else if (action & WINDOW_RESIZE_BOTTOM)
	    cursor = JI_CURSOR_SIZE_BR;
	  else
	    cursor = JI_CURSOR_SIZE_R;
	}
	else if (action & WINDOW_RESIZE_TOP)
	  cursor = JI_CURSOR_SIZE_T;
	else if (action & WINDOW_RESIZE_BOTTOM)
	  cursor = JI_CURSOR_SIZE_B;

	jmouse_set_cursor(cursor);
	return TRUE;
      }
      break;

  }

  return FALSE;
}

static void window_request_size(JWidget widget, int *w, int *h)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW);
  JWidget manager = jwidget_get_manager(widget);

  if (window->is_desktop) {
    JRect cpos = jwidget_get_child_rect(manager);
    *w = jrect_w(cpos);
    *h = jrect_h(cpos);
    jrect_free(cpos);
  }
  else {
    int max_w, max_h;
    int req_w, req_h;
    JWidget child;
    JLink link;

    max_w = max_h = 0;
    JI_LIST_FOR_EACH(widget->children, link) {
      child = (JWidget)link->data;

      if (!jwidget_is_decorative(child)) {
	jwidget_request_size(child, &req_w, &req_h);

	max_w = MAX(max_w, req_w);
	max_h = MAX(max_h, req_h);
      }
    }

    if (widget->text)
      max_w = MAX(max_w, jwidget_get_text_length(widget));

    *w = widget->border_width.l + max_w + widget->border_width.r;
    *h = widget->border_width.t + max_h + widget->border_width.b;
  }
}

static void window_set_position(JWidget widget, JRect rect)
{
  JWidget child;
  JRect cpos;
  JLink link;

  /* copy the new position rectangle */
  jrect_copy(widget->rc, rect);
  cpos = jwidget_get_child_rect(widget);

  /* set all the children to the same "child_pos" */
  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    if (jwidget_is_decorative(child))
      child->theme->map_decorative_widget(child);
    else
      jwidget_set_rect(child, cpos);
  }

  jrect_free(cpos);
}

static int get_action(JWidget widget, int x, int y)
{
  Window *window = jwidget_get_data(widget, JI_WINDOW);
  int action = WINDOW_NONE;
  JRect pos;
  JRect cpos;

  if (!window->is_moveable)
    return action;

  pos = jwidget_get_rect(widget);
  cpos = jwidget_get_child_rect(widget);

  /* move */
  if ((widget->text)
      && (((x >= cpos->x1) &&
	   (x < cpos->x2) &&
	   (y >= pos->y1+widget->border_width.b) &&
	   (y < cpos->y1))
	  || (key_shifts & KB_ALT_FLAG))) {
    action = WINDOW_MOVE;
  }
  /* resize */
  else if (window->is_sizeable) {
    /* left *****************************************/
    if ((x >= pos->x1) && (x < cpos->x1)) {
      action |= WINDOW_RESIZE_LEFT;
      /* top */
      if ((y >= pos->y1) && (y < cpos->y1)) {
	action |= WINDOW_RESIZE_TOP;
      }
      /* bottom */
      else if ((y > cpos->y2-1) && (y <= pos->y2-1))
	action |= WINDOW_RESIZE_BOTTOM;
    }
    /* top *****************************************/
    else if ((y >= pos->y1) && (y < cpos->y1)) {
      action |= WINDOW_RESIZE_TOP;
      /* left */
      if ((x >= pos->x1) && (x < cpos->x1))
	action |= WINDOW_RESIZE_LEFT;
      /* right */
      else if ((x > cpos->x2-1) && (x <= pos->x2-1))
	action |= WINDOW_RESIZE_RIGHT;
    }
    /* right *****************************************/
    else if ((x > cpos->x2-1) && (x <= pos->x2-1)) {
      action |= WINDOW_RESIZE_RIGHT;
      /* top */
      if ((y >= pos->y1) && (y < cpos->y1)) {
	action |= WINDOW_RESIZE_TOP;
      }
      /* bottom */
      else if ((y > cpos->y2-1) && (y <= pos->y2-1))
	action |= WINDOW_RESIZE_BOTTOM;
    }
    /* bottom *****************************************/
    else if ((y > cpos->y2-1) && (y <= pos->y2-1)) {
      action |= WINDOW_RESIZE_BOTTOM;
      /* left */
      if ((x >= pos->x1) && (x < cpos->x1))
	action |= WINDOW_RESIZE_LEFT;
      /* right */
      else if ((x > cpos->x2-1) && (x <= pos->x2-1))
	action |= WINDOW_RESIZE_RIGHT;
    }
  }

  jrect_free(pos);
  jrect_free(cpos);

  return action;
}

static void limit_size(JWidget widget, int *w, int *h)
{
  int req_w, req_h;

  jwidget_request_size(widget, &req_w, &req_h);

  *w = MAX(*w, widget->border_width.l+widget->border_width.r);
  *h = MAX(*h, widget->border_width.t+widget->border_width.b);
}

/* TODO add support to blit available regions */
static void move_window(JWidget widget, JRect rect, bool use_blit)
{
#define FLAGS JI_GDR_CUTTOPWINDOWS | JI_GDR_USECHILDAREA

  JWidget manager = jwidget_get_manager(widget);
  JRegion old_drawable_region;
  JRegion new_drawable_region;
  JRegion manager_refresh_region;
  JRegion window_refresh_region;
  JRect old_pos;
  JRect man_pos;
  JMessage msg;

  jmanager_dispatch_messages(manager);

  /* get the window's current position */
  old_pos = jrect_new_copy(widget->rc);

  /* get the manager's current position */
  man_pos = jwidget_get_rect(manager);

  /* sent a JM_WINMOVE message to the window */
  msg = jmessage_new(JM_WINMOVE);
  jmessage_add_dest(msg, widget);
  jmanager_enqueue_message(msg);

  /* get the region & the drawable region of the window */
  old_drawable_region = jwidget_get_drawable_region(widget, FLAGS);

  /* if the size of the window changes... */
  if (jrect_w(old_pos) != jrect_w(rect) ||
      jrect_h(old_pos) != jrect_h(rect)) {
    /* we have to change the whole positions sending JM_SETPOS
       messages... */
    window_set_position(widget, rect);
  }
  else {
    /* we can just displace all the widgets
       by a delta (new_position - old_position)... */
    displace_widgets(widget,
		     rect->x1 - old_pos->x1,
		     rect->y1 - old_pos->y1);
  }

  /* get the new drawable region of the window (it's new because we
     moved the window to "rect") */
  new_drawable_region = jwidget_get_drawable_region(widget, FLAGS);

  /* create a new region to refresh the manager later */
  manager_refresh_region = jregion_new(NULL, 0);

  /* create a new region to refresh the window later */
  window_refresh_region = jregion_new(NULL, 0);

  /* first of all, we have to refresh the manager in the old window's
     drawable region... */
  jregion_copy(manager_refresh_region, old_drawable_region);

  /* ...but we have to substract the new window's drawable region (and
     that is all for the manager's refresh region) */
  jregion_subtract(manager_refresh_region, manager_refresh_region,
		   new_drawable_region);

  /* now we have to setup the window's refresh region... */

  /* if "use_blit" isn't activated, we have to redraw the whole window
     (sending JM_DRAW messages) in the new drawable region */
  if (!use_blit) {
    jregion_copy(window_refresh_region, new_drawable_region);
  }
  /* if "use_blit" is activated, we can move the old drawable to the
     new position (to redraw as little as possible) */
  else {
    JRegion reg1 = jregion_new(NULL, 0);
    JRegion moveable_region = jregion_new(NULL, 0);

    /* add a region to draw areas which were outside of the screen */
    jregion_copy(reg1, new_drawable_region);
    jregion_translate(reg1,
		      old_pos->x1 - widget->rc->x1,
		      old_pos->y1 - widget->rc->y1);
    jregion_intersect(moveable_region, old_drawable_region, reg1);

    jregion_subtract(reg1, reg1, moveable_region);
    jregion_translate(reg1,
		      widget->rc->x1 - old_pos->x1,
		      widget->rc->y1 - old_pos->y1);
    jregion_union(window_refresh_region, window_refresh_region, reg1);

    /* move the window's graphics */
    jmouse_hide();
    set_clip(ji_screen,
	     man_pos->x1, man_pos->y1, man_pos->x2-1, man_pos->y2-1);

    ji_move_region(moveable_region,
		   widget->rc->x1 - old_pos->x1,
		   widget->rc->y1 - old_pos->y1);
    set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
    jmouse_show();

    jregion_free(reg1);
    jregion_free(moveable_region);
  }

  jwidget_redraw_region(manager, manager_refresh_region);
  jwidget_redraw_region(widget, window_refresh_region);

  jregion_free(old_drawable_region);
  jregion_free(new_drawable_region);
  jregion_free(manager_refresh_region);
  jregion_free(window_refresh_region);
  jrect_free(old_pos);
  jrect_free(man_pos);
}

static void displace_widgets(JWidget widget, int x, int y)
{
  JLink link;

  jrect_displace(widget->rc, x, y);

  JI_LIST_FOR_EACH(widget->children, link)
    displace_widgets(link->data, x, y);
}
