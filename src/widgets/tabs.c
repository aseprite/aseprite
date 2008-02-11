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

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "modules/gfx.h"
#include "modules/gui.h"
#include "widgets/tabs.h"

#define CALC_TAB_WIDTH(widget, tab)			\
  (4 + text_length(widget->text_font, tab->text) + 4)

#define ARROW_W		12

#define HAS_ARROWS ((jwidget_get_parent(tabs->button_left) == widget))

typedef struct Tab
{
  char *text;
  void *data;
  int width;
} Tab;

typedef struct Tabs
{
  JList list_of_tabs;
  Tab *hot;
  Tab *selected;
  void (*select_callback)(void *data);
  int timer_id;
  int scroll_x;
/*   int hot_arrow; */
  JWidget button_left;
  JWidget button_right;
} Tabs;

static int tabs_type(void);
static bool tabs_msg_proc(JWidget widget, JMessage msg);
static bool tabs_button_msg_proc(JWidget widget, JMessage msg);

static Tab *get_tab_by_data(JWidget widget, void *data);
static int get_max_scroll_x(JWidget widget);
static void make_tab_visible(JWidget widget, Tab *tab);
static void set_scroll_x(JWidget widget, int scroll_x);
static void calculate_hot(JWidget widget);
static void draw_bevel_box(JRect box, int c1, int c2, int bottom);

static Tab *tab_new(const char *text, void *data);
static void tab_free(Tab *tab);

/**************************************************************/
/* Tabs                                                       */
/**************************************************************/

JWidget tabs_new(void (*select_callback)(void *data))
{
  JWidget widget = jwidget_new(tabs_type());
  Tabs *tabs = jnew0(Tabs, 1);

  tabs->list_of_tabs = jlist_new();
  tabs->hot = NULL;
  tabs->selected = NULL;
  tabs->select_callback = select_callback;
  tabs->timer_id = jmanager_add_timer(widget, 1000/60);
  tabs->scroll_x = 0;
/*   tabs->hot_arrow = 0; */

  tabs->button_left = jbutton_new(NULL);
  tabs->button_right = jbutton_new(NULL);

  jbutton_set_bevel(tabs->button_left, 0, 0, 0, 0);
  jbutton_set_bevel(tabs->button_right, 0, 0, 0, 0);

  jwidget_focusrest(tabs->button_left, FALSE);
  jwidget_focusrest(tabs->button_right, FALSE);
  
  add_gfxicon_to_button(tabs->button_left, GFX_ARROW_LEFT, JI_CENTER | JI_MIDDLE);
  add_gfxicon_to_button(tabs->button_right, GFX_ARROW_RIGHT, JI_CENTER | JI_MIDDLE);

  jwidget_add_hook(widget, tabs_type(), tabs_msg_proc, tabs);
  jwidget_add_hook(tabs->button_left, tabs_type(), tabs_button_msg_proc, (void *)-1);
  jwidget_add_hook(tabs->button_right, tabs_type(), tabs_button_msg_proc, (void *)+1);

  return widget;
}

void tabs_append_tab(JWidget widget, const char *text, void *data)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  Tab *tab = tab_new(text, data);
  tab->width = CALC_TAB_WIDTH(widget, tab);

  jlist_append(tabs->list_of_tabs, tab);

  /* update scroll (in the same position if we can */
  set_scroll_x(widget, tabs->scroll_x);
  
  jwidget_dirty(widget);
}

void tabs_remove_tab(JWidget widget, void *data)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  Tab *tab = get_tab_by_data(widget, data);

  if (tab) {
    jlist_remove(tabs->list_of_tabs, tab);
    tab_free(tab);

    /* update scroll (in the same position if we can */
    set_scroll_x(widget, tabs->scroll_x);

    jwidget_dirty(widget);
  }
}

void tabs_set_text_for_tab(JWidget widget, const char *text, void *data)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  Tab *tab = get_tab_by_data(widget, data);

  if (tab) {
    /* free old text */
    if (tab->text)
      jfree(tab->text);

    /* change text of the tab */
    tab->text = jstrdup(text);
    tab->width = CALC_TAB_WIDTH(widget, tab);

    /* make it visible (if it's the selected) */
    if (tabs->selected == tab)
      make_tab_visible(widget, tab);
    
    jwidget_dirty(widget);
  }
}

void tabs_select_tab(JWidget widget, void *data)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  Tab *tab = get_tab_by_data(widget, data);

  if (tab) {
    tabs->selected = tab;
    make_tab_visible(widget, tab);
    jwidget_dirty(widget);
  }
}

static int tabs_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool tabs_msg_proc(JWidget widget, JMessage msg)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());

  switch (msg->type) {

    case JM_DESTROY: {
      JLink link;

      JI_LIST_FOR_EACH(tabs->list_of_tabs, link)
	tab_free(link->data);

      jwidget_free(tabs->button_left);
      jwidget_free(tabs->button_right);
      
      jlist_free(tabs->list_of_tabs);

      jmanager_remove_timer(tabs->timer_id);
      jfree(tabs);
      break;
    }

    case JM_REQSIZE:
      msg->reqsize.w =
	msg->reqsize.h = 4 + jwidget_get_text_height(widget) + 5;
      return TRUE;

/*     case JM_SIGNAL: */
/*       if (msg->signal.num == JI_SIGNAL_SHOW) */
/* 	set_scroll_x(widget, tabs->scroll_x); */
/*       break; */

    case JM_SETPOS:
      jrect_copy(widget->rc, &msg->setpos.rect);
      set_scroll_x(widget, tabs->scroll_x);
      return TRUE;

    case JM_DRAW: {
      JRect rect = jwidget_get_rect(widget);
      JRect box = jrect_new(rect->x1-tabs->scroll_x, rect->y1, 0, rect->y2-1);
      JLink link;

      /* for each tab */
      JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
	Tab *tab = link->data;
	int fg, face, bottom;

	box->x2 = box->x1 + tab->width;

	/* is the tab inside the bounds of the widget? */
	if (box->x1 < rect->x2 && box->x2 > rect->x1) {
	  /* selected */
	  if (tabs->selected == tab) {
	    fg = ji_color_background();
	    face = ji_color_selected();
	    bottom = face;
	  }
	  /* non-selected */
	  else {
	    fg = ji_color_foreground();
	    face = tabs->hot == tab ? ji_color_hotface():
				      ji_color_face();
	    bottom = ji_color_facelight();
	  }

	  hline(ji_screen, box->x1, box->y1, box->x2-1, ji_color_face());
	  rectfill(ji_screen, box->x1+1, box->y1+1, box->x2-2, box->y2-1, face);
	  hline(ji_screen, box->x1, rect->y2-1, box->x2-1, ji_color_selected());

	  draw_bevel_box(box,
			 ji_color_facelight(),
			 ji_color_faceshadow(),
			 bottom);

	  jdraw_text(widget->text_font, tab->text,
		     box->x1+4,
		     (box->y1+box->y2)/2-text_height(widget->text_font)/2,
		     fg, face, FALSE);
	}

	box->x1 = box->x2;
      }

      /* fill the gap to the right-side */
      if (box->x1 < rect->x2) {
	rectfill(ji_screen, box->x1, rect->y1, rect->x2-1, rect->y2-3,
		 ji_color_face());
	hline(ji_screen, box->x1, rect->y2-2, rect->x2-1, ji_color_facelight());
	hline(ji_screen, box->x1, rect->y2-1, rect->x2-1, ji_color_selected());
      }

      /* draw bottom lines below the arrow-buttons */
      if (HAS_ARROWS) {
	hline(ji_screen, box->x1, rect->y2-2, rect->x2-1, ji_color_facelight());
	hline(ji_screen, box->x1, rect->y2-1, rect->x2-1, ji_color_selected());
      }

      jrect_free(rect);
      jrect_free(box);
      return TRUE;
    }

    case JM_MOUSEENTER:
    case JM_MOTION:
      calculate_hot(widget);
      return TRUE;

    case JM_MOUSELEAVE:
      if (tabs->hot != NULL) {
	tabs->hot = NULL;
	jwidget_dirty(widget);
      }
      return TRUE;

    case JM_BUTTONPRESSED:
      if (tabs->selected != tabs->hot && tabs->hot != NULL) {
	tabs->selected = tabs->hot;
	jwidget_dirty(widget);

	if (tabs->selected && tabs->select_callback)
	  (*tabs->select_callback)(tabs->selected->data);
      }
      return TRUE;

    case JM_WHEEL: {
      int dx = (jmouse_z(1) - jmouse_z(0)) * jrect_w(widget->rc)/6;
      set_scroll_x(widget, tabs->scroll_x+dx);
      return TRUE;
    }

    case JM_TIMER: {
/*       JWidget parent = jwidget_get_parent(widget); */
/*       Tabs *tabs = jwidget_get_data(parent, tabs_type()); */
/*       int dir = (int)jwidget_get_data(widget, tabs_type()); */
      int dir = jmanager_get_capture() == tabs->button_left ? -1: 1;
      set_scroll_x(widget, tabs->scroll_x + dir*8*msg->timer.count);
      break;
    }

  }

  return FALSE;
}

static bool tabs_button_msg_proc(JWidget widget, JMessage msg)
{
  JWidget parent;
  Tabs *tabs;

  parent = jwidget_get_parent(widget);
  if (parent)
    tabs = jwidget_get_data(parent, tabs_type());

  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
	return TRUE;
      }
      else if (msg->signal.num == JI_SIGNAL_DISABLE) {
	if (jwidget_is_selected(widget)) {
	  jmanager_stop_timer(tabs->timer_id);
	  jwidget_deselect(widget);
	}
	return TRUE;
      }
      break;

    case JM_BUTTONPRESSED:
      jmanager_start_timer(tabs->timer_id);
      break;

    case JM_BUTTONRELEASED:
      jmanager_stop_timer(tabs->timer_id);
      break;

  }

  return FALSE;
}

static Tab *get_tab_by_data(JWidget widget, void *data)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  JLink link;

  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab *tab = link->data;
    if (tab->data == data)
      return tab;
  }

  return NULL;
}

static int get_max_scroll_x(JWidget widget)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  JLink link;
  int x = 0;

  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab *tab = link->data;
    x += tab->width;
  }

  x -= jrect_w(widget->rc);

  if (x < 0)
    return 0;
  else
    return x + ARROW_W*2;
}

static void make_tab_visible(JWidget widget, Tab *make_visible_this_tab)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  JLink link;
  int x = 0;
  int extra_x = get_max_scroll_x(widget) > 0 ? ARROW_W*2: 0;

  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab *tab = link->data;

    if (tab == make_visible_this_tab) {
      if (x - tabs->scroll_x < 0) {
	set_scroll_x(widget, x);
      }
      else if (x + tab->width - tabs->scroll_x > jrect_w(widget->rc) - extra_x) {
	set_scroll_x(widget, x + tab->width - jrect_w(widget->rc) + extra_x);
      }
      break;
    }

    x += tab->width;
  }
}

static void set_scroll_x(JWidget widget, int scroll_x)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  int max_x = get_max_scroll_x(widget);

  scroll_x = MID(0, scroll_x, max_x);
  if (tabs->scroll_x != scroll_x) {
    tabs->scroll_x = scroll_x;
    calculate_hot(widget);
    jwidget_dirty(widget);
  }

  PRINTF("max_x = %d\n", max_x);
  PRINTF("widget->rc->x1 = %d\n", widget->rc->x1);
  PRINTF("widget->rc->y1 = %d\n", widget->rc->y1);
  PRINTF("widget->rc->x2 = %d\n", widget->rc->x2);
  PRINTF("widget->rc->y2 = %d\n", widget->rc->y2);

  /* we need scroll buttons? */
/*   if (max_x > 0 && widget->rc->x2 > 0) { */
  if (max_x > 0) {
    /* add childs */
    if (!HAS_ARROWS) {
      jwidget_add_child(widget, tabs->button_left);
      jwidget_add_child(widget, tabs->button_right);
      jwidget_dirty(widget);
    }

    /* disable/enable buttons */
    if (tabs->scroll_x > 0)
      jwidget_enable(tabs->button_left);
    else
      jwidget_disable(tabs->button_left);

    if (tabs->scroll_x < max_x)
      jwidget_enable(tabs->button_right);
    else
      jwidget_disable(tabs->button_right);

    /* setup the position of each button */
    {
      JRect rect = jwidget_get_rect(widget);
      JRect box = jrect_new(rect->x2-ARROW_W*2, rect->y1,
			    rect->x2-ARROW_W, rect->y2-2);
      jwidget_set_rect(tabs->button_left, box);

      jrect_moveto(box, box->x1+ARROW_W, box->y1);
      jwidget_set_rect(tabs->button_right, box);

      jrect_free(box);
      jrect_free(rect);
    }
  }
  /* remove buttons */
  else if (HAS_ARROWS) {
    jwidget_remove_child(widget, tabs->button_left);
    jwidget_remove_child(widget, tabs->button_right);
    jwidget_dirty(widget);
  }
}

static void calculate_hot(JWidget widget)
{
  Tabs *tabs = jwidget_get_data(widget, tabs_type());
  JRect rect = jwidget_get_rect(widget);
  JRect box = jrect_new(rect->x1-tabs->scroll_x, rect->y1, 0, rect->y2-1);
  JLink link;
  Tab *hot = NULL;

  /* for each tab */
  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab *tab = link->data;

    box->x2 = box->x1 + tab->width;

    if (jrect_point_in(box, jmouse_x(0), jmouse_y(0))) {
      hot = tab;
      break;
    }

    box->x1 = box->x2;
  }

  if (tabs->hot != hot) {
    tabs->hot = hot;
    jwidget_dirty(widget);
  }

  jrect_free(rect);
  jrect_free(box);
}

static void draw_bevel_box(JRect box, int c1, int c2, int bottom)
{
  hline(ji_screen, box->x1+1, box->y1, box->x2-2, c1);	   /* top */
  hline(ji_screen, box->x1, box->y2-1, box->x2-1, bottom); /* bottom */
  vline(ji_screen, box->x1, box->y1+1, box->y2-2, c1);	   /* left */
  vline(ji_screen, box->x2-1, box->y1+1, box->y2-2, c2);   /* right */
}

/**************************************************************/
/* Tab                                                        */
/**************************************************************/

static Tab *tab_new(const char *text, void *data)
{
  Tab *tab = jnew0(Tab, 1);
  if (!tab)
    return NULL;

  tab->text = jstrdup(text);
  tab->data = data;
  tab->width = 0;

  return tab;
}

static void tab_free(Tab *tab)
{
  if (tab->text)
    jfree(tab->text);
  jfree(tab);
}
