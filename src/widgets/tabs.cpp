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
#include <algorithm>
#include <cmath>

#include "jinete/jinete.h"
#include "jinete/jintern.h"

#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/skinneable_theme.h"
#include "widgets/tabs.h"

#define ARROW_W		(12*jguiscale())

#define ANI_ADDING_TAB_TICKS      5
#define ANI_REMOVING_TAB_TICKS    10
#define ANI_SMOOTH_SCROLL_TICKS   20

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
  m_timerId = jmanager_add_timer(this, 1000/60);
  m_scrollX = 0;
  m_ani = ANI_NONE;
  m_removedTab = NULL;

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
  if (m_removedTab) {
    delete m_removedTab;
    m_removedTab = NULL;
  }

  // Stop animation
  stopAni();

  // Remove all tabs
  std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();
  for (it = m_list_of_tabs.begin(); it != end; ++it)
    delete *it; // tab
  m_list_of_tabs.clear();

  delete m_button_left;		// widget
  delete m_button_right;	// widget

  jmanager_remove_timer(m_timerId);
}

void Tabs::addTab(const char* text, void* data)
{
  Tab *tab = new Tab(text, data);
  tab->width = calcTabWidth(tab);

  m_list_of_tabs.push_back(tab);

  // Update scroll (in the same position if we can
  setScrollX(m_scrollX);

  startAni(ANI_ADDING_TAB);
  //jwidget_dirty(this);
}

void Tabs::removeTab(void* data)
{
  Tab *tab = getTabByData(data);

  if (tab) {
    if (m_hot == tab) m_hot = NULL;
    if (m_selected == tab) m_selected = NULL;

    std::vector<Tab*>::iterator it =
      std::find(m_list_of_tabs.begin(), m_list_of_tabs.end(), tab);

    ASSERT(it != m_list_of_tabs.end() && "Removing a tab that is not part of the Tabs widget");

    it = m_list_of_tabs.erase(it);

    // Width of the removed tab
    if (m_removedTab) {
      delete m_removedTab;
      m_removedTab = NULL;
    }
    m_removedTab = tab;

    // Next tab in the list
    if (it != m_list_of_tabs.end())
      m_nextTabOfTheRemovedOne = *it;
    else
      m_nextTabOfTheRemovedOne = NULL;

    // Update scroll (in the same position if we can)
    setScrollX(m_scrollX);

    startAni(ANI_REMOVING_TAB);
  }
}

void Tabs::setTabText(const char* text, void* data)
{
  Tab *tab = getTabByData(data);

  if (tab) {
    // Change text of the tab
    tab->text = text;
    tab->width = calcTabWidth(tab);

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

bool Tabs::onProcessMessage(JMessage msg)
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
      setScrollX(m_scrollX);
      return true;

    case JM_DRAW: {
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      JRect rect = jwidget_get_rect(this);
      jrect_displace(rect, -msg->draw.rect.x1, -msg->draw.rect.y1);
	
      JRect box = jrect_new(rect->x1-m_scrollX,
			    rect->y1,
			    rect->x1-m_scrollX+2*jguiscale(),
			    rect->y1+theme->get_part(PART_TAB_FILLER)->h);

      clear_to_color(doublebuffer, theme->get_window_face_color());

      theme->draw_part_as_hline(doublebuffer, box->x1, box->y1, box->x2-1, box->y2-1, PART_TAB_FILLER);
      theme->draw_part_as_hline(doublebuffer, box->x1, box->y2, box->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);

      box->x1 = box->x2;

      // For each tab...
      std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

      for (it = m_list_of_tabs.begin(); it != end; ++it) {
	Tab* tab = *it;
	
	box->x2 = box->x1 + tab->width;

	int x_delta = 0;
	int y_delta = 0;

	// Y-delta for animating tabs (intros and outros)
	if (m_ani == ANI_ADDING_TAB && m_selected == tab) {
	  y_delta = (box->y2 - box->y1) * (ANI_ADDING_TAB_TICKS - m_ani_t) / ANI_ADDING_TAB_TICKS;
	}
	else if (m_ani == ANI_REMOVING_TAB && m_nextTabOfTheRemovedOne == tab) {
	  x_delta += m_removedTab->width - m_removedTab->width*(1.0-std::exp(-10.0 * m_ani_t / (double)ANI_REMOVING_TAB_TICKS));
	  x_delta = MID(0, x_delta, m_removedTab->width);

	  // Draw deleted tab
	  if (m_removedTab) {
	    JRect box2 = jrect_new(box->x1, box->y1, box->x1+x_delta, box->y2);
	    drawTab(doublebuffer, box2, m_removedTab, 0, false);
	    jrect_free(box2);
	  }
	}

	box->x1 += x_delta;
	box->x2 += x_delta;
	
	drawTab(doublebuffer, box, tab, y_delta, (tab == m_selected));

	box->x1 = box->x2;
      }

      if (m_ani == ANI_REMOVING_TAB && m_nextTabOfTheRemovedOne == NULL) {
	// Draw deleted tab
	if (m_removedTab) {
	  int x_delta = m_removedTab->width - m_removedTab->width*(1.0-std::exp(-10.0 * m_ani_t / (double)ANI_REMOVING_TAB_TICKS));
	  x_delta = MID(0, x_delta, m_removedTab->width);

	  JRect box2 = jrect_new(box->x1, box->y1, box->x1+x_delta, box->y2);
	  drawTab(doublebuffer, box2, m_removedTab, 0, false);
	  jrect_free(box2);

	  box->x1 += x_delta;
	  box->x2 = box->x1;
	}
      }

      /* fill the gap to the right-side */
      if (box->x1 < rect->x2) {
	theme->draw_part_as_hline(doublebuffer, box->x1, box->y1, rect->x2-1, box->y2-1, PART_TAB_FILLER);
	theme->draw_part_as_hline(doublebuffer, box->x1, box->y2, rect->x2-1, rect->y2-1, PART_TAB_BOTTOM_NORMAL);
      }

      jrect_free(rect);
      jrect_free(box);

      blit(doublebuffer, ji_screen, 0, 0,
	   msg->draw.rect.x1,
	   msg->draw.rect.y1,
	   doublebuffer->w,
	   doublebuffer->h);
      destroy_bitmap(doublebuffer);
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
      // setScrollX(m_scrollX+dx);

      m_begScrollX = m_scrollX;
      if (m_ani != ANI_SMOOTH_SCROLL)
	m_endScrollX = m_scrollX + dx;
      else
	m_endScrollX += dx;

      // Limit endScrollX position (to improve animation ending to the correct position)
      {
	int max_x = getMaxScrollX();
	m_endScrollX = MID(0, m_endScrollX, max_x);
      }

      startAni(ANI_SMOOTH_SCROLL);
      return true;
    }

    case JM_TIMER: {
      switch (m_ani) {
	case ANI_NONE:
	  // Do nothing
	  break;
	case ANI_SCROLL: {
	  int dir = jmanager_get_capture() == m_button_left ? -1: 1;
	  setScrollX(m_scrollX + dir*8*msg->timer.count);
	  break;
	}
	case ANI_SMOOTH_SCROLL: {
	  if (m_ani_t == ANI_SMOOTH_SCROLL_TICKS) {
	    stopAni();
	    setScrollX(m_endScrollX);
	  }
	  else {
	    // Lineal
	    //setScrollX(m_begScrollX + m_endScrollX - m_begScrollX) * m_ani_t / 10);

	    // Exponential
	    setScrollX(m_begScrollX +
		       (m_endScrollX - m_begScrollX) * (1.0-std::exp(-10.0 * m_ani_t / (double)ANI_SMOOTH_SCROLL_TICKS)));
	  }
	  break;
	}
	case ANI_ADDING_TAB: {
	  if (m_ani_t == ANI_ADDING_TAB_TICKS)
	    stopAni();
	  dirty();
	  break;
	}
	case ANI_REMOVING_TAB: {
	  if (m_ani_t == ANI_REMOVING_TAB_TICKS)
	    stopAni();
	  dirty();
	  break;
	}
      }
      ++m_ani_t;
      break;
    }

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	m_button_left->setBgColor(theme->get_tab_selected_face_color());
	m_button_right->setBgColor(theme->get_tab_selected_face_color());
      }
      else if (msg->signal.num == JI_SIGNAL_SET_FONT) {
	std::vector<Tab*>::iterator it, end = m_list_of_tabs.end();

	for (it = m_list_of_tabs.begin(); it != end; ++it) {
	  Tab* tab = *it;
	  tab->width = calcTabWidth(tab);
	}
      }
      else if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	/* setup the background color */
	jwidget_set_bg_color(this, ji_color_face());
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void Tabs::drawTab(BITMAP* bmp, JRect box, Tab* tab, int y_delta, bool selected)
{
  // Is the tab outside the bounds of the widget?
  if (box->x1 >= this->rc->x2 || box->x2 <= this->rc->x1)
    return;

  SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
  int text_color;
  int face_color;

  // Selected
  if (selected) {
    text_color = theme->get_tab_selected_text_color();
    face_color = theme->get_tab_selected_face_color();
  }
  // Non-selected
  else {
    text_color = theme->get_tab_normal_text_color();
    face_color = theme->get_tab_normal_face_color();
  }

  if (jrect_w(box) > 2) {
    theme->draw_bounds_nw(bmp,
			  box->x1, box->y1+y_delta, box->x2-1, box->y2-1,
			  (selected) ? PART_TAB_SELECTED_NW:
				       PART_TAB_NORMAL_NW, face_color);
    jdraw_text(bmp, this->getFont(), tab->text.c_str(),
	       box->x1+4*jguiscale(),
	       (box->y1+box->y2)/2-text_height(this->getFont())/2+1 + y_delta,
	       text_color, face_color, false, jguiscale());
  }

  if (selected) {
    theme->draw_bounds_nw(bmp,
			  box->x1, box->y2, box->x2-1, this->rc->y2-1,
			  PART_TAB_BOTTOM_SELECTED_NW,
			  theme->get_tab_selected_face_color());
  }
  else {
    theme->draw_part_as_hline(bmp,
			      box->x1, box->y2, box->x2-1, this->rc->y2-1,
			      PART_TAB_BOTTOM_NORMAL);
  }

#ifdef CLOSE_BUTTON_IN_EACH_TAB
  BITMAP* close_icon = theme->get_part(PART_WINDOW_CLOSE_BUTTON_NORMAL);
  set_alpha_blender();
  draw_trans_sprite(doublebuffer, close_icon,
		    box->x2-4*jguiscale()-close_icon->w,
		    (box->y1+box->y2)/2-close_icon->h/2+1*jguiscale());
#endif
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
      if (x - m_scrollX < 0) {
	setScrollX(x);
      }
      else if (x + tab->width - m_scrollX > jrect_w(this->rc) - extra_x) {
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
  if (m_scrollX != scroll_x) {
    m_scrollX = scroll_x;
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
    m_button_left->setEnabled(m_scrollX > 0);
    m_button_right->setEnabled(m_scrollX < max_x);

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
  JRect box = jrect_new(rect->x1-m_scrollX, rect->y1, 0, rect->y2-1);
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

int Tabs::calcTabWidth(Tab* tab)
{
  int border = 4*jguiscale();
#ifdef CLOSE_BUTTON_IN_EACH_TAB
  SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
  int close_icon_w = theme->get_part(PART_WINDOW_CLOSE_BUTTON_NORMAL)->w;
  return (border + text_length(getFont(), tab->text.c_str()) + border + close_icon_w + border);
#else
  return (border + text_length(getFont(), tab->text.c_str()) + border);
#endif
}

void Tabs::startScrolling()
{
  startAni(ANI_SCROLL);
}

void Tabs::stopScrolling()
{
  stopAni();
}

void Tabs::startAni(Ani ani)
{
  // Stop previous animation
  if (m_ani != ANI_NONE)
    stopAni();

  m_ani = ani;
  m_ani_t = 0;
  jmanager_start_timer(m_timerId);
}

void Tabs::stopAni()
{
  m_ani = ANI_NONE;
  jmanager_stop_timer(m_timerId);
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
	ASSERT(tabs != NULL);

	if (widget->isSelected()) {
	  tabs->stopScrolling();
	  widget->setSelected(false);
	}
	return true;
      }
      break;

    case JM_BUTTONPRESSED:
      ASSERT(tabs != NULL);
      tabs->startScrolling();
      break;

    case JM_BUTTONRELEASED:
      ASSERT(tabs != NULL);
      tabs->stopScrolling();
      break;

  }

  return false;
}

