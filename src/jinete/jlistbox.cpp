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

#include "config.h"

#include <allegro/keyboard.h>

#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"
#include "Vaca/Size.h"

static bool listbox_msg_proc(JWidget widget, JMessage msg);
static void listbox_request_size(JWidget widget, int *w, int *h);
static void listbox_set_position(JWidget widget, JRect rect);
static void listbox_dirty_children(JWidget widget);

static bool listitem_msg_proc(JWidget widget, JMessage msg);
static void listitem_request_size(JWidget widget, int *w, int *h);

JWidget jlistbox_new()
{
  JWidget widget = new Widget(JI_LISTBOX);

  jwidget_add_hook(widget, JI_LISTBOX, listbox_msg_proc, NULL);
  jwidget_focusrest(widget, true);
  jwidget_init_theme(widget);

  return widget;
}

JWidget jlistitem_new(const char *text)
{
  JWidget widget = new Widget(JI_LISTITEM);

  jwidget_add_hook(widget, JI_LISTITEM, listitem_msg_proc, NULL);
  widget->setAlign(JI_LEFT | JI_MIDDLE);
  widget->setText(text);
  jwidget_init_theme(widget);

  return widget;
}

JWidget jlistbox_get_selected_child(JWidget widget)
{
  JLink link;
  JI_LIST_FOR_EACH(widget->children, link) {
    if (((JWidget)link->data)->isSelected())
      return (JWidget)link->data;
  }
  return 0;
}

int jlistbox_get_selected_index(JWidget widget)
{
  JLink link;
  int i = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    if (((JWidget)link->data)->isSelected())
      return i;
    i++;
  }

  return -1;
}

void jlistbox_select_child(JWidget widget, JWidget listitem)
{
  JWidget child;
  JLink link;

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    if (child->isSelected()) {
      if ((listitem) && (child == listitem))
	return;

      child->setSelected(false);
    }
  }

  if (listitem) {
    JWidget view = jwidget_get_view(widget);

    listitem->setSelected(true);

    if (view) {
      JRect vp = jview_get_viewport_position(view);
      int scroll_x, scroll_y;

      jview_get_scroll(view, &scroll_x, &scroll_y);

      if (listitem->rc->y1 < vp->y1)
	jview_set_scroll(view, scroll_x, listitem->rc->y1 - widget->rc->y1);
      else if (listitem->rc->y1 > vp->y2 - jrect_h(listitem->rc))
	jview_set_scroll(view, scroll_x,
			 listitem->rc->y1 - widget->rc->y1
			 - jrect_h(vp) + jrect_h(listitem->rc));

      jrect_free(vp);
    }
  }

  jwidget_emit_signal(widget, JI_SIGNAL_LISTBOX_CHANGE);
}

void jlistbox_select_index(JWidget widget, int index)
{
  JWidget child = reinterpret_cast<JWidget>(jlist_nth_data(widget->children, index));
  if (child)
    jlistbox_select_child(widget, child);
}

int jlistbox_get_items_count(JWidget widget)
{
  return jlist_length(widget->children);
}

/* setup the scroll to center the selected item in the viewport */
void jlistbox_center_scroll(JWidget widget)
{
  JWidget view = jwidget_get_view(widget);
  JWidget listitem = jlistbox_get_selected_child(widget);

  if (view && listitem) {
    JRect vp = jview_get_viewport_position(view);
    int scroll_x, scroll_y;

    jview_get_scroll(view, &scroll_x, &scroll_y);
    jview_set_scroll(view,
		     scroll_x,
		     (listitem->rc->y1 - widget->rc->y1)
		     - jrect_h(vp)/2 + jrect_h(listitem->rc)/2);
    jrect_free(vp);
  }
}

static bool listbox_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      listbox_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS:
      listbox_set_position(widget, &msg->setpos.rect);
      return true;

    case JM_DRAW:
      widget->theme->draw_listbox(widget, &msg->draw.rect);
      return true;

    case JM_DIRTYCHILDREN:
      listbox_dirty_children(widget);
      return true;

    case JM_OPEN:
      jlistbox_center_scroll(widget);
      break;

    case JM_BUTTONPRESSED:
      widget->captureMouse();

    case JM_MOTION:
      if (widget->hasCapture()) {
	int select = jlistbox_get_selected_index(widget);
	JWidget view = jwidget_get_view(widget);
	bool pick_item = true;

	if (view) {
	  JRect vp = jview_get_viewport_position(view);

	  if (msg->mouse.y < vp->y1) {
	    int num = MAX(1, (vp->y1 - msg->mouse.y) / 8);
	    jlistbox_select_index(widget, select-num);
	    pick_item = false;
	  }
	  else if (msg->mouse.y >= vp->y2) {
	    int num = MAX(1, (msg->mouse.y - (vp->y2-1)) / 8);
	    jlistbox_select_index(widget, select+num);
	    pick_item = false;
	  }

	  jrect_free(vp);
	}

	if (pick_item) {
	  JWidget picked;

	  if (view) {
	    picked = jwidget_pick(jview_get_viewport(view),
				  msg->mouse.x, msg->mouse.y);
	  }
	  else {
	    picked = jwidget_pick(widget, msg->mouse.x, msg->mouse.y);
	  }

	  /* if the picked widget is a child of the list, select it */
	  if (picked && jwidget_has_child(widget, picked))
	    jlistbox_select_child(widget, picked);
	}

	return true;
      }
      break;

    case JM_BUTTONRELEASED:
      widget->releaseMouse();
      break;

    case JM_WHEEL: {
      JWidget view = jwidget_get_view(widget);
      if (view) {
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	jview_set_scroll(view,
			 scroll_x,
			 scroll_y +
			 (jmouse_z(1) - jmouse_z(0))
			 *jwidget_get_text_height(widget)*3);
      }
      break;
    }

    case JM_KEYPRESSED:
      if (widget->hasFocus() && !jlist_empty(widget->children)) {
	int select = jlistbox_get_selected_index(widget);
	JWidget view = jwidget_get_view(widget);
	int bottom = MAX(0, jlist_length(widget->children)-1);

	switch (msg->key.scancode) {
	  case KEY_UP:
	    select--;
	    break;
	  case KEY_DOWN:
	    select++;
	    break;
	  case KEY_HOME:
	    select = 0;
	    break;
	  case KEY_END:
	    select = bottom;
	    break;
	  case KEY_PGUP:
	    if (view) {
	      JRect vp = jview_get_viewport_position(view);
	      select -= jrect_h(vp) / jwidget_get_text_height(widget);
	      jrect_free(vp);
	    }
	    else
	      select = 0;
	    break;
	  case KEY_PGDN:
	    if (view) {
	      JRect vp = jview_get_viewport_position(view);
	      select += jrect_h(vp) / jwidget_get_text_height(widget);
	      jrect_free(vp);
	    }
	    else
	      select = bottom;
	    break;
	  case KEY_LEFT:
	  case KEY_RIGHT:
	    if (view) {
	      JRect vp = jview_get_viewport_position(view);
	      int sgn = (msg->key.scancode == KEY_LEFT) ? -1: 1;
	      int scroll_x, scroll_y;

	      jview_get_scroll(view, &scroll_x, &scroll_y);
	      jview_set_scroll(view, scroll_x + jrect_w(vp)/2*sgn, scroll_y);
	      jrect_free(vp);
	    }
	    break;
	  default:
	    return false;
	}

	jlistbox_select_index(widget, MID(0, select, bottom));
	return true;
      }
      break;

    case JM_DOUBLECLICK:
      jwidget_emit_signal(widget, JI_SIGNAL_LISTBOX_SELECT);
      return true;
  }

  return false;
}

static void listbox_request_size(JWidget widget, int *w, int *h)
{
  Size reqSize;
  JLink link;

  *w = *h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    reqSize = reinterpret_cast<Widget*>(link->data)->getPreferredSize();

    *w = MAX(*w, reqSize.w);
    *h += reqSize.h + ((link->next)? widget->child_spacing: 0);
  }

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void listbox_set_position(JWidget widget, JRect rect)
{
  Size reqSize;
  JWidget child;
  JRect cpos;
  JLink link;

  jrect_copy(widget->rc, rect);
  cpos = jwidget_get_child_rect(widget);

  JI_LIST_FOR_EACH(widget->children, link) {
    child = (JWidget)link->data;

    reqSize = child->getPreferredSize();

    cpos->y2 = cpos->y1+reqSize.h;
    jwidget_set_rect(child, cpos);

    cpos->y1 += jrect_h(child->rc) + widget->child_spacing;
  }

  jrect_free(cpos);
}

static void listbox_dirty_children(JWidget widget)
{
  JWidget view = jwidget_get_view(widget);
  JWidget child;
  JLink link;
  JRect vp;

  if (!view) {
    JI_LIST_FOR_EACH(widget->children, link)
      jwidget_dirty(reinterpret_cast<JWidget>(link->data));
  }
  else {
    vp = jview_get_viewport_position(view);

    JI_LIST_FOR_EACH(widget->children, link) {
      child = reinterpret_cast<JWidget>(link->data);

      if (child->rc->y2 <= vp->y1)
	continue;
      else if (child->rc->y1 >= vp->y2)
	break;

      jwidget_dirty(child);
    }

    jrect_free(vp);
  }
}

static bool listitem_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      listitem_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_SETPOS: {
      JRect crect;
      JLink link;

      jrect_copy(widget->rc, &msg->setpos.rect);
      crect = jwidget_get_child_rect(widget);

      JI_LIST_FOR_EACH(widget->children, link)
	jwidget_set_rect(reinterpret_cast<JWidget>(link->data), crect);

      jrect_free(crect);
      return true;
    }

    case JM_DRAW:
      widget->theme->draw_listitem(widget, &msg->draw.rect);
      return true;
  }

  return false;
}

static void listitem_request_size(JWidget widget, int *w, int *h)
{
  Size maxSize;
  Size reqSize;
  JLink link;

  if (widget->hasText()) {
    maxSize.w = jwidget_get_text_length(widget);
    maxSize.h = jwidget_get_text_height(widget);
  }
  else
    maxSize.w = maxSize.h = 0;

  JI_LIST_FOR_EACH(widget->children, link) {
    reqSize = reinterpret_cast<Widget*>(link->data)->getPreferredSize();

    maxSize.w = MAX(maxSize.w, reqSize.w);
    maxSize.h = MAX(maxSize.h, reqSize.h);
  }

  *w = widget->border_width.l + maxSize.w + widget->border_width.r;
  *h = widget->border_width.t + maxSize.h + widget->border_width.b;
}
