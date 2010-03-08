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
#include <assert.h>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/skinneable_theme.h"
#include "widgets/tabs.h"

#define CALC_TAB_WIDTH(widget, tab)				\
  (4 + text_length(widget->getFont(), tab->text.c_str()) + 4)

#define ARROW_W		12

#define HAS_ARROWS ((jwidget_get_parent(tabs->button_left) == widget))

struct Tab
{
  std::string text;
  void *data;
  int width;

  Tab(const char* text, void* data)
  {
    this->text = text;
    this->data = data;
    this->width = 0;
  }
};

typedef struct Tabs
{
  JList list_of_tabs;
  Tab *hot;
  Tab *selected;
  void (*select_callback)(JWidget tabs, void *data, int button);
  int timer_id;
  int scroll_x;
/*   int hot_arrow; */
  JWidget button_left;
  JWidget button_right;
} Tabs;

static int tabs_type();
static bool tabs_msg_proc(JWidget widget, JMessage msg);
static bool tabs_button_msg_proc(JWidget widget, JMessage msg);

static Tab *get_tab_by_data(JWidget widget, void *data);
static int get_max_scroll_x(JWidget widget);
static void make_tab_visible(JWidget widget, Tab *tab);
static void set_scroll_x(JWidget widget, int scroll_x);
static void calculate_hot(JWidget widget);
static void draw_bevel_box(JRect box, int c1, int c2, int bottom);

/**************************************************************/
/* Tabs                                                       */
/**************************************************************/

JWidget tabs_new(void (*select_callback)(JWidget tabs, void *data, int button))
{
  Widget* widget = new Widget(tabs_type());
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

  jwidget_focusrest(tabs->button_left, false);
  jwidget_focusrest(tabs->button_right, false);
  
  add_gfxicon_to_button(tabs->button_left, GFX_ARROW_LEFT, JI_CENTER | JI_MIDDLE);
  add_gfxicon_to_button(tabs->button_right, GFX_ARROW_RIGHT, JI_CENTER | JI_MIDDLE);

  jwidget_add_hook(widget, tabs_type(), tabs_msg_proc, tabs);
  jwidget_add_hook(tabs->button_left, tabs_type(), tabs_button_msg_proc, (void *)-1);
  jwidget_add_hook(tabs->button_right, tabs_type(), tabs_button_msg_proc, (void *)+1);

  jwidget_init_theme(widget);

  return widget;
}

void tabs_append_tab(JWidget widget, const char *text, void *data)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  Tab *tab = new Tab(text, data);
  tab->width = CALC_TAB_WIDTH(widget, tab);

  jlist_append(tabs->list_of_tabs, tab);

  /* update scroll (in the same position if we can */
  set_scroll_x(widget, tabs->scroll_x);
  
  jwidget_dirty(widget);
}

void tabs_remove_tab(JWidget widget, void *data)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  Tab *tab = get_tab_by_data(widget, data);

  if (tab) {
    if (tabs->hot == tab) tabs->hot = NULL;
    if (tabs->selected == tab) tabs->selected = NULL;

    jlist_remove(tabs->list_of_tabs, tab);
    delete tab;

    /* update scroll (in the same position if we can */
    set_scroll_x(widget, tabs->scroll_x);

    jwidget_dirty(widget);
  }
}

void tabs_set_text_for_tab(JWidget widget, const char *text, void *data)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  Tab *tab = get_tab_by_data(widget, data);

  if (tab) {
    /* change text of the tab */
    tab->text = text;
    tab->width = CALC_TAB_WIDTH(widget, tab);

    /* make it visible (if it's the selected) */
    if (tabs->selected == tab)
      make_tab_visible(widget, tab);
    
    jwidget_dirty(widget);
  }
}

void tabs_select_tab(JWidget widget, void *data)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  Tab *tab = get_tab_by_data(widget, data);

  if (tab != NULL) {
    tabs->selected = tab;
    make_tab_visible(widget, tab);
    jwidget_dirty(widget);
  }
}

void* tabs_get_selected_tab(JWidget widget)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));

  if (tabs->selected != NULL)
    return tabs->selected->data;
  else
    return NULL;
}

static int tabs_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

static bool tabs_msg_proc(JWidget widget, JMessage msg)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  SkinneableTheme* theme = static_cast<SkinneableTheme*>(widget->theme);

  switch (msg->type) {

    case JM_DESTROY: {
      JLink link;

      JI_LIST_FOR_EACH(tabs->list_of_tabs, link)
	delete reinterpret_cast<Tab*>(link->data); // tab

      jwidget_free(tabs->button_left);
      jwidget_free(tabs->button_right);
      
      jlist_free(tabs->list_of_tabs);

      jmanager_remove_timer(tabs->timer_id);
      jfree(tabs);
      break;
    }

    case JM_REQSIZE:
      msg->reqsize.w = 0; // msg->reqsize.h = 4 + jwidget_get_text_height(widget) + 5;
      msg->reqsize.h =
	theme->get_part(PART_TAB_FILLER)->h +
	theme->get_part(PART_TAB_BOTTOM_NORMAL)->h;
      return true;

    case JM_SETPOS:
      jrect_copy(widget->rc, &msg->setpos.rect);
      set_scroll_x(widget, tabs->scroll_x);
      return true;

    case JM_DRAW: {
      JRect rect = jwidget_get_rect(widget);
      JRect box = jrect_new(rect->x1-tabs->scroll_x, rect->y1,
			    rect->x1-tabs->scroll_x+2, rect->y1+theme->get_part(PART_TAB_FILLER)->h);
      JLink link;

      theme->draw_hline(box->x1, box->y1, box->x2-1, box->y2-1, PART_TAB_FILLER);
      theme->draw_hline(box->x1, box->y2, box->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);

      box->x1 = box->x2;

      /* for each tab */
      JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
	Tab* tab = reinterpret_cast<Tab*>(link->data);

	box->x2 = box->x1 + tab->width;

	/* is the tab inside the bounds of the widget? */
	if (box->x1 < rect->x2 && box->x2 > rect->x1) {
	  int text_color;
	  int face_color;

	  /* selected */
	  if (tabs->selected == tab) {
	    text_color = theme->get_tab_selected_text_color();
	    face_color = theme->get_tab_selected_face_color();
	  }
	  /* non-selected */
	  else {
	    text_color = theme->get_tab_normal_text_color();
	    face_color = theme->get_tab_normal_face_color();
	  }

	  theme->draw_bounds(box->x1, box->y1, box->x2-1, box->y2-1,
			     (tabs->selected == tab) ? PART_TAB_SELECTED_NW:
						       PART_TAB_NORMAL_NW, face_color);

	  if (tabs->selected == tab)
	    theme->draw_bounds(box->x1, box->y2, box->x2-1, rect->y2-1,
			       PART_TAB_BOTTOM_SELECTED_NW,
			       theme->get_tab_selected_face_color());
	  else
	    theme->draw_hline(box->x1, box->y2, box->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);
	  
	  jdraw_text(widget->getFont(), tab->text.c_str(),
		     box->x1+4,
		     (box->y1+box->y2)/2-text_height(widget->getFont())/2+1,
		     text_color, face_color, false, guiscale());
	}

	box->x1 = box->x2;
      }

      /* fill the gap to the right-side */
      if (box->x1 < rect->x2) {
	theme->draw_hline(box->x1, box->y1, rect->x2-1, box->y2-1, PART_TAB_FILLER);
	theme->draw_hline(box->x1, box->y2, rect->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);
      }

      jrect_free(rect);
      jrect_free(box);
      return true;
    }

    case JM_MOUSEENTER:
    case JM_MOTION:
      calculate_hot(widget);
      return true;

    case JM_MOUSELEAVE:
      if (tabs->hot != NULL) {
	tabs->hot = NULL;
	jwidget_dirty(widget);
      }
      return true;

    case JM_BUTTONPRESSED:
      if (tabs->hot != NULL) {
	if (tabs->selected != tabs->hot) {
	  tabs->selected = tabs->hot;
	  jwidget_dirty(widget);
	}

	if (tabs->selected && tabs->select_callback)
	  (*tabs->select_callback)(widget,
				   tabs->selected->data,
				   msg->mouse.flags);
      }
      return true;

    case JM_WHEEL: {
      int dx = (jmouse_z(1) - jmouse_z(0)) * jrect_w(widget->rc)/6;
      set_scroll_x(widget, tabs->scroll_x+dx);
      return true;
    }

    case JM_TIMER: {
      int dir = jmanager_get_capture() == tabs->button_left ? -1: 1;
      set_scroll_x(widget, tabs->scroll_x + dir*8*msg->timer.count);
      break;
    }

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_SET_FONT) {
	JLink link;

	JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
	  Tab* tab = reinterpret_cast<Tab*>(link->data);
	  tab->width = CALC_TAB_WIDTH(widget, tab);
	}
      }
      else if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	/* setup the background color */
	jwidget_set_bg_color(widget, ji_color_face());
      }
      break;

  }

  return false;
}

static bool tabs_button_msg_proc(JWidget widget, JMessage msg)
{
  JWidget parent;
  Tabs *tabs = NULL;

  parent = jwidget_get_parent(widget);
  if (parent)
    tabs = reinterpret_cast<Tabs*>(jwidget_get_data(parent, tabs_type()));

  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
	return true;
      }
      else if (msg->signal.num == JI_SIGNAL_DISABLE) {
	assert(tabs != NULL);

	if (jwidget_is_selected(widget)) {
	  jmanager_stop_timer(tabs->timer_id);
	  jwidget_deselect(widget);
	}
	return true;
      }
      break;

    case JM_BUTTONPRESSED:
      assert(tabs != NULL);
      jmanager_start_timer(tabs->timer_id);
      break;

    case JM_BUTTONRELEASED:
      assert(tabs != NULL);
      jmanager_stop_timer(tabs->timer_id);
      break;

  }

  return false;
}

static Tab *get_tab_by_data(JWidget widget, void *data)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  JLink link;

  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab* tab = reinterpret_cast<Tab*>(link->data);
    if (tab->data == data)
      return tab;
  }

  return NULL;
}

static int get_max_scroll_x(JWidget widget)
{
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  JLink link;
  int x = 0;

  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab* tab = reinterpret_cast<Tab*>(link->data);
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
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  JLink link;
  int x = 0;
  int extra_x = get_max_scroll_x(widget) > 0 ? ARROW_W*2: 0;

  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab* tab = reinterpret_cast<Tab*>(link->data);

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
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  int max_x = get_max_scroll_x(widget);

  scroll_x = MID(0, scroll_x, max_x);
  if (tabs->scroll_x != scroll_x) {
    tabs->scroll_x = scroll_x;
    calculate_hot(widget);
    jwidget_dirty(widget);
  }

  /* we need scroll buttons? */
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
  Tabs* tabs = reinterpret_cast<Tabs*>(jwidget_get_data(widget, tabs_type()));
  JRect rect = jwidget_get_rect(widget);
  JRect box = jrect_new(rect->x1-tabs->scroll_x, rect->y1, 0, rect->y2-1);
  JLink link;
  Tab *hot = NULL;

  /* for each tab */
  JI_LIST_FOR_EACH(tabs->list_of_tabs, link) {
    Tab* tab = reinterpret_cast<Tab*>(link->data);

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
