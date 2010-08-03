/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
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
 *   * Neither the name of the author nor the names of its contributors may
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

#include "config.h"

#include <allegro.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"
#include "Vaca/Size.h"
#include "Vaca/PreferredSizeEvent.h"

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

static void displace_widgets(JWidget widget, int x, int y);

bool _jwindow_is_moving()
{
  return (window_action == WINDOW_MOVE) ? true: false;
}

Frame::Frame(bool desktop, const char* text)
  : Widget(JI_FRAME)
{
  m_killer = NULL;
  m_is_desktop = desktop;
  m_is_moveable = !desktop;
  m_is_sizeable = !desktop;
  m_is_ontop = false;
  m_is_wantfocus = true;
  m_is_foreground = false;
  m_is_autoremap = true;

  setVisible(false);
  setText(text);
  setAlign(JI_LEFT | JI_MIDDLE);

  jwidget_init_theme(this);
}

Frame::~Frame()
{
  _jmanager_close_window(getManager(), this, false);
}

Widget* Frame::get_killer()
{
  return m_killer;
}

void Frame::set_autoremap(bool state)
{
  m_is_autoremap = state;
}

void Frame::set_moveable(bool state)
{
  m_is_moveable = state;
}

void Frame::set_sizeable(bool state)
{
  m_is_sizeable = state;
}

void Frame::set_ontop(bool state)
{
  m_is_ontop = state;
}

void Frame::set_wantfocus(bool state)
{
  m_is_wantfocus = state;
}

void Frame::remap_window()
{
  Size reqSize;
  JRect rect;

  if (m_is_autoremap) {
    m_is_autoremap = false;
    this->setVisible(true);
  }

  reqSize = this->getPreferredSize();

  rect = jrect_new(this->rc->x1, this->rc->y1,
		   this->rc->x1+reqSize.w,
		   this->rc->y1+reqSize.h);
  jwidget_set_rect(this, rect);
  jrect_free(rect);

  jwidget_emit_signal(this, JI_SIGNAL_WINDOW_RESIZE);
  jwidget_dirty(this);
}

void Frame::center_window()
{
  JWidget manager = getManager();

  if (m_is_autoremap)
    this->remap_window();

  position_window(jrect_w(manager->rc)/2 - jrect_w(this->rc)/2,
		  jrect_h(manager->rc)/2 - jrect_h(this->rc)/2);
}

void Frame::position_window(int x, int y)
{
  int old_action = window_action;
  JRect rect;

  window_action = WINDOW_MOVE;

  if (m_is_autoremap)
    remap_window();

  rect = jrect_new(x, y, x+jrect_w(this->rc), y+jrect_h(this->rc));
  jwidget_set_rect(this, rect);
  jrect_free(rect);

  window_action = old_action;

  dirty();
}

void Frame::move_window(JRect rect)
{
  move_window(rect, true);
}

void Frame::open_window()
{
  if (!this->parent) {
    if (m_is_autoremap)
      center_window();

    _jmanager_open_window(ji_get_default_manager(), this);
  }
}

void Frame::open_window_fg()
{
  open_window();

  JWidget manager = getManager();

  m_is_foreground = true;

  while (!(this->flags & JI_HIDDEN)) {
    if (jmanager_generate_messages(manager))
      jmanager_dispatch_messages(manager);
  }

  m_is_foreground = false;
}

void Frame::open_window_bg()
{
  this->open_window();
}

void Frame::closeWindow(Widget* killer)
{
  m_killer = killer;

  _jmanager_close_window(getManager(), this, true);
}

bool Frame::is_toplevel()
{
  JWidget manager = getManager();

  if (!jlist_empty(manager->children))
    return (this == jlist_first(manager->children)->data);
  else
    return false;
}

bool Frame::is_foreground() const
{
  return m_is_foreground;
}

bool Frame::is_desktop() const
{
  return m_is_desktop;
}

bool Frame::is_ontop() const
{
  return m_is_ontop;
}

bool Frame::is_wantfocus() const
{
  return m_is_wantfocus;
}

bool Frame::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_SETPOS:
      this->window_set_position(&msg->setpos.rect);
      return true;

    case JM_OPEN:
      m_killer = NULL;
      break;

    case JM_CLOSE:
      // Fire Close signal
      {
	Vaca::CloseEvent ev;
	Close(ev);
      }
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_SET_TEXT)
	jwidget_init_theme(this);
      break;

    case JM_BUTTONPRESSED: {
      if (!m_is_moveable)
	break;

      press_x = msg->mouse.x;
      press_y = msg->mouse.y;
      window_action = this->get_action(press_x, press_y);
      if (window_action != WINDOW_NONE) {
	if (click_pos == NULL)
	  click_pos = jrect_new_copy(this->rc);
	else
	  jrect_copy(click_pos, this->rc);

	captureMouse();
	return true;
      }
      else
	break;
    }

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
	releaseMouse();
	jmouse_set_cursor(JI_CURSOR_NORMAL);

	if (click_pos != NULL) {
	  jrect_free(click_pos);
	  click_pos = NULL;
	}

	window_action = WINDOW_NONE;
	return true;
      }
      break;

    case JM_MOTION:
      if (!m_is_moveable)
	break;

      /* does it have the mouse captured? */
      if (hasCapture()) {
	/* reposition/resize */
	if (window_action == WINDOW_MOVE) {
	  int x = click_pos->x1 + (msg->mouse.x - press_x);
	  int y = click_pos->y1 + (msg->mouse.y - press_y);
	  JRect rect = jrect_new(x, y,
				 x+jrect_w(this->rc),
				 y+jrect_h(this->rc));
	  this->move_window(rect, true);
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

	  this->limit_size(&w, &h);

	  if ((jrect_w(this->rc) != w) ||
	      (jrect_h(this->rc) != h)) {
	    if (window_action & WINDOW_RESIZE_LEFT)
	      x = click_pos->x1 - (w - jrect_w(click_pos));
	    else
	      x = this->rc->x1;

	    if (window_action & WINDOW_RESIZE_TOP)
	      y = click_pos->y1 - (h - jrect_h(click_pos));
	    else
	      y = this->rc->y1;

	    {
	      JRect rect = jrect_new(x, y, x+w, y+h); 
	      this->move_window(rect, false);
	      jrect_free(rect);

	      jwidget_emit_signal(this, JI_SIGNAL_WINDOW_RESIZE);
	      jwidget_dirty(this);
	    }
	  }
	}
      }

      /* TODO */
/*       { */
/* 	JWidget manager = get_manager(); */
/* 	JWidget view = jwidget_get_view(manager); */
/* 	if (view) { */
/* 	  jview_update(view); */
/* 	} */
/*       } */
      break;

    case JM_SETCURSOR:
      if (m_is_moveable) {
	int action = this->get_action(msg->mouse.x, msg->mouse.y);
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
	return true;
      }
      break;

    case JM_DRAW:
      this->theme->draw_frame(this, &msg->draw.rect);
      return true;

  }

  return Widget::onProcessMessage(msg);
}

void Frame::onPreferredSize(PreferredSizeEvent& ev)
{
  JWidget manager = getManager();

  if (m_is_desktop) {
    JRect cpos = jwidget_get_child_rect(manager);
    ev.setPreferredSize(jrect_w(cpos),
			jrect_h(cpos));
    jrect_free(cpos);
  }
  else {
    Size maxSize(0, 0);
    Size reqSize;
    JWidget child;
    JLink link;

    JI_LIST_FOR_EACH(this->children, link) {
      child = (JWidget)link->data;

      if (!jwidget_is_decorative(child)) {
	reqSize = child->getPreferredSize();

	maxSize.w = MAX(maxSize.w, reqSize.w);
	maxSize.h = MAX(maxSize.h, reqSize.h);
      }
    }

    if (this->hasText())
      maxSize.w = MAX(maxSize.w, jwidget_get_text_length(this));

    ev.setPreferredSize(this->border_width.l + maxSize.w + this->border_width.r,
			this->border_width.t + maxSize.h + this->border_width.b);
  }
}

void Frame::window_set_position(JRect rect)
{
  JWidget child;
  JRect cpos;
  JLink link;

  /* copy the new position rectangle */
  jrect_copy(this->rc, rect);
  cpos = jwidget_get_child_rect(this);

  /* set all the children to the same "child_pos" */
  JI_LIST_FOR_EACH(this->children, link) {
    child = (JWidget)link->data;

    if (jwidget_is_decorative(child))
      child->theme->map_decorative_widget(child);
    else
      jwidget_set_rect(child, cpos);
  }

  jrect_free(cpos);
}

int Frame::get_action(int x, int y)
{
  int action = WINDOW_NONE;
  JRect pos;
  JRect cpos;

  if (!m_is_moveable)
    return action;

  pos = jwidget_get_rect(this);
  cpos = jwidget_get_child_rect(this);

  /* move */
  if ((this->hasText())
      && (((x >= cpos->x1) &&
	   (x < cpos->x2) &&
	   (y >= pos->y1+this->border_width.b) &&
	   (y < cpos->y1))
	  || (key_shifts & KB_ALT_FLAG))) {
    action = WINDOW_MOVE;
  }
  /* resize */
  else if (m_is_sizeable) {
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

void Frame::limit_size(int *w, int *h)
{
  *w = MAX(*w, this->border_width.l+this->border_width.r);
  *h = MAX(*h, this->border_width.t+this->border_width.b);
}

void Frame::move_window(JRect rect, bool use_blit)
{
#define FLAGS JI_GDR_CUTTOPWINDOWS | JI_GDR_USECHILDAREA

  JWidget manager = getManager();
  JRegion old_drawable_region;
  JRegion new_drawable_region;
  JRegion manager_refresh_region;
  JRegion window_refresh_region;
  JRect old_pos;
  JRect man_pos;
  JMessage msg;

  jmanager_dispatch_messages(manager);

  /* get the window's current position */
  old_pos = jrect_new_copy(this->rc);

  /* get the manager's current position */
  man_pos = jwidget_get_rect(manager);

  /* sent a JM_WINMOVE message to the window */
  msg = jmessage_new(JM_WINMOVE);
  jmessage_add_dest(msg, this);
  jmanager_enqueue_message(msg);

  /* get the region & the drawable region of the window */
  old_drawable_region = jwidget_get_drawable_region(this, FLAGS);

  /* if the size of the window changes... */
  if (jrect_w(old_pos) != jrect_w(rect) ||
      jrect_h(old_pos) != jrect_h(rect)) {
    /* we have to change the whole positions sending JM_SETPOS
       messages... */
    window_set_position(rect);
  }
  else {
    /* we can just displace all the widgets
       by a delta (new_position - old_position)... */
    displace_widgets(this,
		     rect->x1 - old_pos->x1,
		     rect->y1 - old_pos->y1);
  }

  /* get the new drawable region of the window (it's new because we
     moved the window to "rect") */
  new_drawable_region = jwidget_get_drawable_region(this, FLAGS);

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
		      old_pos->x1 - this->rc->x1,
		      old_pos->y1 - this->rc->y1);
    jregion_intersect(moveable_region, old_drawable_region, reg1);

    jregion_subtract(reg1, reg1, moveable_region);
    jregion_translate(reg1,
		      this->rc->x1 - old_pos->x1,
		      this->rc->y1 - old_pos->y1);
    jregion_union(window_refresh_region, window_refresh_region, reg1);

    /* move the window's graphics */
    jmouse_hide();
    set_clip_rect(ji_screen,
		  man_pos->x1, man_pos->y1, man_pos->x2-1, man_pos->y2-1);

    ji_move_region(moveable_region,
		   this->rc->x1 - old_pos->x1,
		   this->rc->y1 - old_pos->y1);
    set_clip_rect(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
    jmouse_show();

    jregion_free(reg1);
    jregion_free(moveable_region);
  }

  jmanager_invalidate_region(manager, manager_refresh_region);
  jwidget_invalidate_region(this, window_refresh_region);

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
    displace_widgets(reinterpret_cast<JWidget>(link->data), x, y);
}
