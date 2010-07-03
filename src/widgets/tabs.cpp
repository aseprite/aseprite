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
  (4*jguiscale() + text_length(widget->getFont(), tab->text.c_str()) + 4*jguiscale())

#define ARROW_W		(12*jguiscale())

#define HAS_ARROWS(tabs) ((jwidget_get_parent(m_button_left) == (tabs)))

static bool tabs_button_msg_proc(JWidget widget, JMessage msg);

static int tabs_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

Tabs::Tabs(ITabsHandler* handler)
  : Widget(tabs_type())
  , m_handler(handler)
{
  m_hot = NULL;
  m_selected = NULL;
  m_timer_id = jmanager_add_timer(this, 1000/60);
  m_scroll_x = 0;

  m_button_left = jbutton_new(NULL);
  m_button_right = jbutton_new(NULL);

  setup_mini_look(m_button_left);
  setup_mini_look(m_button_right);

  jbutton_set_bevel(m_button_left, 0, 0, 0, 0);
  jbutton_set_bevel(m_button_right, 0, 0, 0, 0);

  jwidget_focusrest(m_button_left, false);
  jwidget_focusrest(m_button_right, false);
  
  add_gfxicon_to_button(m_button_left, GFX_ARROW_LEFT, JI_CENTER | JI_MIDDLE);
  add_gfxicon_to_button(m_button_right, GFX_ARROW_RIGHT, JI_CENTER | JI_MIDDLE);

  jwidget_add_hook(m_button_left, tabs_type(), tabs_button_msg_proc, (void *)-1);
  jwidget_add_hook(m_button_right, tabs_type(), tabs_button_msg_proc, (void *)+1);

  jwidget_init_theme(this);
}

Tabs::~Tabs()
{
  std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

  for (it = m_list_of_tabs.begin(); it != end; ++it)
    delete *it; // tab
  m_list_of_tabs.clear();

  delete m_button_left;		// widget
  delete m_button_right;	// widget

  jmanager_remove_timer(m_timer_id);
}

void Tabs::addTab(const char* text, void* data)
{
  Tab *tab = new Tab(text, data);
  tab->width = CALC_TAB_WIDTH(this, tab);

  m_list_of_tabs.push_back(tab);

  // Update scroll (in the same position if we can
  setScrollX(m_scroll_x);
  
  jwidget_dirty(this);
}

void Tabs::removeTab(void* data)
{
  Tab *tab = getTabByData(data);

  if (tab) {
    if (m_hot == tab) m_hot = NULL;
    if (m_selected == tab) m_selected = NULL;

    Vaca::remove_from_container(m_list_of_tabs, tab);
    delete tab;

    // Update scroll (in the same position if we can
    setScrollX(m_scroll_x);

    jwidget_dirty(this);
  }
}

void Tabs::setTabText(const char* text, void* data)
{
  Tab *tab = getTabByData(data);

  if (tab) {
    // Change text of the tab
    tab->text = text;
    tab->width = CALC_TAB_WIDTH(this, tab);

    // Make it visible (if it's the selected)
    if (m_selected == tab)
      makeTabVisible(tab);
    
    jwidget_dirty(this);
  }
}

void Tabs::selectTab(void* data)
{
  Tab *tab = getTabByData(data);

  if (tab != NULL) {
    m_selected = tab;
    makeTabVisible(tab);
    jwidget_dirty(this);
  }
}

void* Tabs::getSelectedTab()
{
  if (m_selected != NULL)
    return m_selected->data;
  else
    return NULL;
}

int Tabs::getTimerId()
{
  return m_timer_id;
}

bool Tabs::msg_proc(JMessage msg)
{
  SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);

  switch (msg->type) {

    case JM_REQSIZE:
      msg->reqsize.w = 0; // msg->reqsize.h = 4 + jwidget_get_text_height(widget) + 5;
      msg->reqsize.h =
	theme->get_part(PART_TAB_FILLER)->h +
	theme->get_part(PART_TAB_BOTTOM_NORMAL)->h;
      return true;

    case JM_SETPOS:
      jrect_copy(this->rc, &msg->setpos.rect);
      setScrollX(m_scroll_x);
      return true;

    case JM_DRAW: {
      JRect rect = jwidget_get_rect(this);
      JRect box = jrect_new(rect->x1-m_scroll_x, rect->y1,
			    rect->x1-m_scroll_x+2*jguiscale(),
			    rect->y1+theme->get_part(PART_TAB_FILLER)->h);

      theme->draw_part_as_hline(ji_screen, box->x1, box->y1, box->x2-1, box->y2-1, PART_TAB_FILLER);
      theme->draw_part_as_hline(ji_screen, box->x1, box->y2, box->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);

      box->x1 = box->x2;

      // For each tab...
      std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

      for (it = m_list_of_tabs.begin(); it != end; ++it) {
	Tab* tab = *it;

	box->x2 = box->x1 + tab->width;

	/* is the tab inside the bounds of the widget? */
	if (box->x1 < rect->x2 && box->x2 > rect->x1) {
	  int text_color;
	  int face_color;

	  /* selected */
	  if (m_selected == tab) {
	    text_color = theme->get_tab_selected_text_color();
	    face_color = theme->get_tab_selected_face_color();
	  }
	  /* non-selected */
	  else {
	    text_color = theme->get_tab_normal_text_color();
	    face_color = theme->get_tab_normal_face_color();
	  }

	  theme->draw_bounds_nw(ji_screen,
				box->x1, box->y1, box->x2-1, box->y2-1,
				(m_selected == tab) ? PART_TAB_SELECTED_NW:
							  PART_TAB_NORMAL_NW, face_color);

	  if (m_selected == tab) {
	    theme->draw_bounds_nw(ji_screen,
				  box->x1, box->y2, box->x2-1, rect->y2-1,
				  PART_TAB_BOTTOM_SELECTED_NW,
				  theme->get_tab_selected_face_color());
	  }
	  else {
	    theme->draw_part_as_hline(ji_screen,
				      box->x1, box->y2, box->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);
	  }
	  
	  jdraw_text(this->getFont(), tab->text.c_str(),
		     box->x1+4*jguiscale(),
		     (box->y1+box->y2)/2-text_height(this->getFont())/2+1,
		     text_color, face_color, false, jguiscale());
	}

	box->x1 = box->x2;
      }

      /* fill the gap to the right-side */
      if (box->x1 < rect->x2) {
	theme->draw_part_as_hline(ji_screen, box->x1, box->y1, rect->x2-1, box->y2-1, PART_TAB_FILLER);
	theme->draw_part_as_hline(ji_screen, box->x1, box->y2, rect->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);
      }

      jrect_free(rect);
      jrect_free(box);
      return true;
    }

    case JM_MOUSEENTER:
    case JM_MOTION:
      calculateHot();
      return true;

    case JM_MOUSELEAVE:
      if (m_hot != NULL) {
	m_hot = NULL;
	jwidget_dirty(this);
      }
      return true;

    case JM_BUTTONPRESSED:
      if (m_hot != NULL) {
	if (m_selected != m_hot) {
	  m_selected = m_hot;
	  jwidget_dirty(this);
	}

	if (m_selected && m_handler)
	  m_handler->clickTab(this,
			      m_selected->data,
			      msg->mouse.flags);
      }
      return true;

    case JM_WHEEL: {
      int dx = (jmouse_z(1) - jmouse_z(0)) * jrect_w(this->rc)/6;
      setScrollX(m_scroll_x+dx);
      return true;
    }

    case JM_TIMER: {
      int dir = jmanager_get_capture() == m_button_left ? -1: 1;
      setScrollX(m_scroll_x + dir*8*msg->timer.count);
      break;
    }

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_SET_FONT) {
	std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

	for (it = m_list_of_tabs.begin(); it != end; ++it) {
	  Tab* tab = *it;
	  tab->width = CALC_TAB_WIDTH(this, tab);
	}
      }
      else if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	/* setup the background color */
	jwidget_set_bg_color(this, ji_color_face());
      }
      break;

  }

  return Widget::msg_proc(msg);
}

Tabs::Tab* Tabs::getTabByData(void* data)
{
  std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;
    if (tab->data == data)
      return tab;
  }

  return NULL;
}

int Tabs::getMaxScrollX()
{
  std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();
  int x = 0;

  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;
    x += tab->width;
  }

  x -= jrect_w(this->rc);

  if (x < 0)
    return 0;
  else
    return x + ARROW_W*2;
}

void Tabs::makeTabVisible(Tab* make_visible_this_tab)
{
  int x = 0;
  int extra_x = getMaxScrollX() > 0 ? ARROW_W*2: 0;
  std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;

    if (tab == make_visible_this_tab) {
      if (x - m_scroll_x < 0) {
	setScrollX(x);
      }
      else if (x + tab->width - m_scroll_x > jrect_w(this->rc) - extra_x) {
	setScrollX(x + tab->width - jrect_w(this->rc) + extra_x);
      }
      break;
    }

    x += tab->width;
  }
}

void Tabs::setScrollX(int scroll_x)
{
  int max_x = getMaxScrollX();

  scroll_x = MID(0, scroll_x, max_x);
  if (m_scroll_x != scroll_x) {
    m_scroll_x = scroll_x;
    calculateHot();
    jwidget_dirty(this);
  }

  // We need scroll buttons?
  if (max_x > 0) {
    // Add childs
    if (!HAS_ARROWS(this)) {
      jwidget_add_child(this, m_button_left);
      jwidget_add_child(this, m_button_right);
      jwidget_dirty(this);
    }

    /* disable/enable buttons */
    if (m_scroll_x > 0)
      jwidget_enable(m_button_left);
    else
      jwidget_disable(m_button_left);

    if (m_scroll_x < max_x)
      jwidget_enable(m_button_right);
    else
      jwidget_disable(m_button_right);

    /* setup the position of each button */
    {
      JRect rect = jwidget_get_rect(this);
      JRect box = jrect_new(rect->x2-ARROW_W*2, rect->y1,
			    rect->x2-ARROW_W, rect->y2-2);
      jwidget_set_rect(m_button_left, box);

      jrect_moveto(box, box->x1+ARROW_W, box->y1);
      jwidget_set_rect(m_button_right, box);

      jrect_free(box);
      jrect_free(rect);
    }
  }
  // Remove buttons
  else if (HAS_ARROWS(this)) {
    jwidget_remove_child(this, m_button_left);
    jwidget_remove_child(this, m_button_right);
    jwidget_dirty(this);
  }
}

void Tabs::calculateHot()
{
  JRect rect = jwidget_get_rect(this);
  JRect box = jrect_new(rect->x1-m_scroll_x, rect->y1, 0, rect->y2-1);
  Tab *hot = NULL;
  std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

  // For each tab
  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;

    box->x2 = box->x1 + tab->width;

    if (jrect_point_in(box, jmouse_x(0), jmouse_y(0))) {
      hot = tab;
      break;
    }

    box->x1 = box->x2;
  }

  if (m_hot != hot) {
    m_hot = hot;

    if (m_handler)
      m_handler->mouseOverTab(this, m_hot ? m_hot->data: NULL);

    jwidget_dirty(this);
  }

  jrect_free(rect);
  jrect_free(box);
}

static bool tabs_button_msg_proc(JWidget widget, JMessage msg)
{
  JWidget parent;
  Tabs* tabs = NULL;

  parent = jwidget_get_parent(widget);
  if (parent)
    tabs = dynamic_cast<Tabs*>(parent);

  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
	return true;
      }
      else if (msg->signal.num == JI_SIGNAL_DISABLE) {
	assert(tabs != NULL);

	if (widget->isSelected()) {
	  jmanager_stop_timer(tabs->getTimerId());
	  widget->setSelected(false);
	}
	return true;
      }
      break;

    case JM_BUTTONPRESSED:
      assert(tabs != NULL);
      jmanager_start_timer(tabs->getTimerId());
      break;

    case JM_BUTTONRELEASED:
      assert(tabs != NULL);
      jmanager_stop_timer(tabs->getTimerId());
      break;

  }

  return false;
}

