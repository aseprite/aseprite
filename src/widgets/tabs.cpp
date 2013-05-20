/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include <algorithm>
#include <allegro.h>
#include <cmath>

#include "modules/gfx.h"
#include "modules/gui.h"
#include "skin/skin_theme.h"
#include "ui/gui.h"
#include "ui/intern.h"
#include "widgets/tabs.h"

using namespace ui;

#define ARROW_W         (12*jguiscale())

#define ANI_ADDING_TAB_TICKS      5
#define ANI_REMOVING_TAB_TICKS    10
#define ANI_SMOOTH_SCROLL_TICKS   20

#define HAS_ARROWS(tabs) ((m_button_left->getParent() == (tabs)))

static WidgetType tabs_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

class Tabs::ScrollButton : public Button
{
public:
  ScrollButton(int direction, Tabs* tabs)
    : Button("")
    , m_direction(direction)
    , m_tabs(tabs) {
  }

  int getDirection() const { return m_direction; }

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onDisable() OVERRIDE;

private:
  int m_direction;
  Tabs* m_tabs;
};

Tabs::Tabs(TabsDelegate* delegate)
  : Widget(tabs_type())
  , m_delegate(delegate)
  , m_timer(1000/60, this)
{
  setDoubleBuffered(true);

  m_hot = NULL;
  m_selected = NULL;
  m_scrollX = 0;
  m_ani = ANI_NONE;
  m_removedTab = NULL;

  m_button_left = new ScrollButton(-1, this);
  m_button_right = new ScrollButton(+1, this);

  setup_mini_look(m_button_left);
  setup_mini_look(m_button_right);
  setup_bevels(m_button_left, 2, 0, 2, 0);
  setup_bevels(m_button_right, 0, 2, 0, 2);

  m_button_left->setFocusStop(false);
  m_button_right->setFocusStop(false);

  set_gfxicon_to_button(m_button_left,
                        PART_COMBOBOX_ARROW_LEFT,
                        PART_COMBOBOX_ARROW_LEFT_SELECTED,
                        PART_COMBOBOX_ARROW_LEFT_DISABLED, JI_CENTER | JI_MIDDLE);

  set_gfxicon_to_button(m_button_right,
                        PART_COMBOBOX_ARROW_RIGHT,
                        PART_COMBOBOX_ARROW_RIGHT_SELECTED,
                        PART_COMBOBOX_ARROW_RIGHT_DISABLED, JI_CENTER | JI_MIDDLE);

  initTheme();
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
  TabsListIterator it, end = m_list_of_tabs.end();
  for (it = m_list_of_tabs.begin(); it != end; ++it)
    delete *it; // tab
  m_list_of_tabs.clear();

  delete m_button_left;         // widget
  delete m_button_right;        // widget
}

void Tabs::addTab(TabView* tabView)
{
  Tab* tab = new Tab(tabView);
  calcTabWidth(tab);

  m_list_of_tabs.push_back(tab);

  // Update scroll (in the same position if we can
  setScrollX(m_scrollX);

  startAni(ANI_ADDING_TAB);
}

void Tabs::removeTab(TabView* tabView)
{
  Tab* tab = getTabByView(tabView);
  if (!tab)
    return;

  if (m_hot == tab) m_hot = NULL;
  if (m_selected == tab) m_selected = NULL;

  TabsListIterator it =
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

void Tabs::updateTabsText()
{
  TabsListIterator it, end = m_list_of_tabs.end();

  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;

    // Change text of the tab
    calcTabWidth(tab);
  }
  invalidate();
}

void Tabs::selectTab(TabView* tabView)
{
  ASSERT(tabView != NULL);

  Tab *tab = getTabByView(tabView);
  if (tab != NULL)
    selectTabInternal(tab);
}

void Tabs::selectNextTab()
{
  TabsListIterator currentTabIt = getTabIteratorByView(m_selected->view);
  TabsListIterator it = currentTabIt;
  if (it != m_list_of_tabs.end()) {
    // If we are at the end of the list, cycle to the first tab.
    if (it == --m_list_of_tabs.end())
      it = m_list_of_tabs.begin();
    // Go to next tab.
    else
      ++it;

    if (it != currentTabIt) {
      selectTabInternal(*it);
      if (m_delegate)
        m_delegate->clickTab(this, m_selected->view, 1);
    }
  }
}

void Tabs::selectPreviousTab()
{
  TabsListIterator currentTabIt = getTabIteratorByView(m_selected->view);
  TabsListIterator it = currentTabIt;
  if (it != m_list_of_tabs.end()) {
    // If we are at the beginning of the list, cycle to the last tab.
    if (it == m_list_of_tabs.begin())
      it = --m_list_of_tabs.end();
    // Go to previous tab.
    else
      --it;

    if (it != currentTabIt) {
      selectTabInternal(*it);
      if (m_delegate)
        m_delegate->clickTab(this, m_selected->view, 1);
    }
  }
}

TabView* Tabs::getSelectedTab()
{
  if (m_selected != NULL)
    return m_selected->view;
  else
    return NULL;
}

bool Tabs::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case kMouseEnterMessage:
    case kMouseMoveMessage:
      calculateHot();
      return true;

    case kMouseLeaveMessage:
      if (m_hot != NULL) {
        m_hot = NULL;
        invalidate();
      }
      return true;

    case kMouseDownMessage:
      if (m_hot != NULL) {
        if (m_selected != m_hot) {
          m_selected = m_hot;
          invalidate();
        }

        if (m_selected && m_delegate)
          m_delegate->clickTab(this,
                               m_selected->view,
                               msg->mouse.flags);
      }
      return true;

    case kMouseWheelMessage: {
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

    case kTimerMessage: {
      switch (m_ani) {
        case ANI_NONE:
          // Do nothing
          break;
        case ANI_SCROLL: {
          ScrollButton* button = dynamic_cast<ScrollButton*>(getManager()->getCapture());
          if (button != NULL)
            setScrollX(m_scrollX + button->getDirection()*8*msg->timer.count);
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
          invalidate();
          break;
        }
        case ANI_REMOVING_TAB: {
          if (m_ani_t == ANI_REMOVING_TAB_TICKS)
            stopAni();
          invalidate();
          break;
        }
      }
      ++m_ani_t;
      break;
    }

  }

  return Widget::onProcessMessage(msg);
}

void Tabs::onPaint(PaintEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  Graphics* g = ev.getGraphics();
  gfx::Rect rect = getClientBounds();
  gfx::Rect box(rect.x-m_scrollX,
                rect.y,
                2*jguiscale(),
                theme->get_part(PART_TAB_FILLER)->h);

  g->fillRect(theme->getColor(ThemeColor::WindowFace), g->getClipBounds());

  theme->draw_part_as_hline(g, box, PART_TAB_FILLER);
  theme->draw_part_as_hline(g, gfx::Rect(box.x, box.y2(), box.w, rect.y2()-box.y2()), PART_TAB_BOTTOM_NORMAL);

  box.x = box.x2();

  // For each tab...
  TabsListIterator it, end = m_list_of_tabs.end();

  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;

    box.w = tab->width;

    int x_delta = 0;
    int y_delta = 0;

    // Y-delta for animating tabs (intros and outros)
    if (m_ani == ANI_ADDING_TAB && m_selected == tab) {
      y_delta = box.h * (ANI_ADDING_TAB_TICKS - m_ani_t) / ANI_ADDING_TAB_TICKS;
    }
    else if (m_ani == ANI_REMOVING_TAB && m_nextTabOfTheRemovedOne == tab) {
      x_delta += m_removedTab->width - m_removedTab->width*(1.0-std::exp(-10.0 * m_ani_t / (double)ANI_REMOVING_TAB_TICKS));
      x_delta = MID(0, x_delta, m_removedTab->width);

      // Draw deleted tab
      if (m_removedTab) {
        gfx::Rect box2(box.x, box.y, x_delta, box.h);
        drawTab(g, box2, m_removedTab, 0, false);
      }
    }

    box.x += x_delta;
    drawTab(g, box, tab, y_delta, (tab == m_selected));

    box.x = box.x2();
  }

  if (m_ani == ANI_REMOVING_TAB && m_nextTabOfTheRemovedOne == NULL) {
    // Draw deleted tab
    if (m_removedTab) {
      int x_delta = m_removedTab->width - m_removedTab->width*(1.0-std::exp(-10.0 * m_ani_t / (double)ANI_REMOVING_TAB_TICKS));
      x_delta = MID(0, x_delta, m_removedTab->width);

      gfx::Rect box2(box.x, box.y, x_delta, box.h);
      drawTab(g, box2, m_removedTab, 0, false);

      box.x += x_delta;
      box.w = 0;
    }
  }

  // Fill the gap to the right-side
  if (box.x < rect.x2()) {
    theme->draw_part_as_hline(g, gfx::Rect(box.x, box.y, rect.x2()-box.x, box.h), PART_TAB_FILLER);
    theme->draw_part_as_hline(g, gfx::Rect(box.x, box.y2(), rect.x2()-box.x, rect.y2()-box.y2()), PART_TAB_BOTTOM_NORMAL);
  }
}

void Tabs::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());
  setScrollX(m_scrollX);
}

void Tabs::onPreferredSize(PreferredSizeEvent& ev)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());

  ev.setPreferredSize(gfx::Size(0, // 4 + jwidget_get_text_height(widget) + 5,
                                theme->get_part(PART_TAB_FILLER)->h +
                                theme->get_part(PART_TAB_BOTTOM_NORMAL)->h));
}

void Tabs::onInitTheme(InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);

  SkinTheme* theme = static_cast<SkinTheme*>(ev.getTheme());

  m_button_left->setBgColor(theme->getColor(ThemeColor::TabSelectedFace));
  m_button_right->setBgColor(theme->getColor(ThemeColor::TabSelectedFace));
}

void Tabs::onSetText()
{
  TabsListIterator it, end = m_list_of_tabs.end();

  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;
    calcTabWidth(tab);
  }
}

void Tabs::selectTabInternal(Tab* tab)
{
  m_selected = tab;
  makeTabVisible(tab);
  invalidate();
}

void Tabs::drawTab(Graphics* g, const gfx::Rect& box, Tab* tab, int y_delta, bool selected)
{
  // Is the tab outside the bounds of the widget?
  if (box.x >= this->rc->x2 || box.x2() <= this->rc->x1)
    return;

  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  ui::Color text_color;
  ui::Color face_color;

  // Selected
  if (selected) {
    text_color = theme->getColor(ThemeColor::TabSelectedText);
    face_color = theme->getColor(ThemeColor::TabSelectedFace);
  }
  // Non-selected
  else {
    text_color = theme->getColor(ThemeColor::TabNormalText);
    face_color = theme->getColor(ThemeColor::TabNormalFace);
  }

  if (box.w > 2) {
    theme->draw_bounds_nw(g,
                          gfx::Rect(box.x, box.y+y_delta, box.w, box.h),
                          (selected) ? PART_TAB_SELECTED_NW:
                                       PART_TAB_NORMAL_NW,
                          face_color);

    g->drawString(tab->text, text_color, face_color, false,
                  gfx::Point(box.x + 4*jguiscale(),
                             box.y + box.h/2 - text_height(this->getFont())/2+1 + y_delta));
  }

  if (selected) {
    theme->draw_bounds_nw(g, gfx::Rect(box.x, box.y2(), box.w, this->rc->y2-box.y2()),
                          PART_TAB_BOTTOM_SELECTED_NW,
                          theme->getColor(ThemeColor::TabSelectedFace));
  }
  else {
    theme->draw_part_as_hline(g, gfx::Rect(box.x, box.y2(), box.w, this->rc->y2-box.y2()),
                              PART_TAB_BOTTOM_NORMAL);
  }

#ifdef CLOSE_BUTTON_IN_EACH_TAB
  BITMAP* close_icon = theme->get_part(PART_WINDOW_CLOSE_BUTTON_NORMAL);
  g->drawAlphaBitmap(close_icon,
                     box.x2() - 4*jguiscale() - close_icon->w,
                     box.y + box.h/2 - close_icon->h/2+1 * jguiscale());
#endif
}

Tabs::TabsListIterator Tabs::getTabIteratorByView(TabView* tabView)
{
  TabsListIterator it, end = m_list_of_tabs.end();

  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    if ((*it)->view == tabView)
      break;
  }

  return it;
}

Tabs::Tab* Tabs::getTabByView(TabView* tabView)
{
  TabsListIterator it = getTabIteratorByView(tabView);
  if (it != m_list_of_tabs.end())
    return *it;
  else
    return NULL;
}

int Tabs::getMaxScrollX()
{
  TabsListIterator it, end = m_list_of_tabs.end();
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
  TabsListIterator it, end = m_list_of_tabs.end();

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
    invalidate();
  }

  // We need scroll buttons?
  if (max_x > 0) {
    // Add childs
    if (!HAS_ARROWS(this)) {
      addChild(m_button_left);
      addChild(m_button_right);
      invalidate();
    }

    /* disable/enable buttons */
    m_button_left->setEnabled(m_scrollX > 0);
    m_button_right->setEnabled(m_scrollX < max_x);

    // Setup the position of each button
    gfx::Rect rect = getBounds();
    gfx::Rect box(rect.x2()-ARROW_W*2, rect.y, ARROW_W, rect.h-2);
    m_button_left->setBounds(box);

    box.x += ARROW_W;
    m_button_right->setBounds(box);
  }
  // Remove buttons
  else if (HAS_ARROWS(this)) {
    removeChild(m_button_left);
    removeChild(m_button_right);
    invalidate();
  }
}

void Tabs::calculateHot()
{
  gfx::Rect rect = getBounds();
  gfx::Rect box(rect.x-m_scrollX, rect.y, 0, rect.h-1);
  Tab *hot = NULL;
  TabsListIterator it, end = m_list_of_tabs.end();

  // For each tab
  for (it = m_list_of_tabs.begin(); it != end; ++it) {
    Tab* tab = *it;

    box.w = tab->width;

    if (box.contains(gfx::Point(jmouse_x(0), jmouse_y(0)))) {
      hot = tab;
      break;
    }

    box.x += box.w;
  }

  if (m_hot != hot) {
    m_hot = hot;

    if (m_delegate)
      m_delegate->mouseOverTab(this, m_hot ? m_hot->view: NULL);

    invalidate();
  }
}

void Tabs::calcTabWidth(Tab* tab)
{
  // Cache current tab text
  tab->text = tab->view->getTabText();

  int border = 4*jguiscale();
#ifdef CLOSE_BUTTON_IN_EACH_TAB
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme);
  int close_icon_w = theme->get_part(PART_WINDOW_CLOSE_BUTTON_NORMAL)->w;
  tab->width = (border + text_length(getFont(), tab->text.c_str()) + border + close_icon_w + border);
#else
  tab->width = (border + text_length(getFont(), tab->text.c_str()) + border);
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
  m_timer.start();
}

void Tabs::stopAni()
{
  m_ani = ANI_NONE;
  m_timer.stop();
}

bool Tabs::ScrollButton::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case kMouseDownMessage:
      captureMouse();
      m_tabs->startScrolling();
      return true;

    case kMouseUpMessage:
      if (hasCapture())
        releaseMouse();

      m_tabs->stopScrolling();
      return true;

  }

  return Button::onProcessMessage(msg);
}

void Tabs::ScrollButton::onDisable()
{
  Button::onDisable();

  if (hasCapture())
    releaseMouse();

  if (isSelected()) {
    m_tabs->stopScrolling();
    setSelected(false);
  }
}
