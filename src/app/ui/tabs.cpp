// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "app/ui/tabs.h"
#include "she/font.h"
#include "she/surface.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <algorithm>
#include <cmath>

#define ARROW_W         (12*guiscale())

#define ANI_ADDING_TAB_TICKS      5
#define ANI_REMOVING_TAB_TICKS    10
#define ANI_SMOOTH_SCROLL_TICKS   20

#define HAS_ARROWS(tabs) ((m_button_left->getParent() == (tabs)))

namespace app {

using namespace app::skin;
using namespace ui;

static WidgetType tabs_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

Tabs::Tabs(TabsDelegate* delegate)
  : Widget(tabs_type())
  , m_delegate(delegate)
  , m_timer(1000/60, this)
{
  setDoubleBuffered(true);

  m_hot = NULL;
  m_hotCloseButton = false;
  m_selected = NULL;
  m_scrollX = 0;
  m_ani = ANI_NONE;
  m_removedTab = NULL;

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
  for (Tab* tab : m_list)
    delete tab;
  m_list.clear();
}

void Tabs::addTab(TabView* tabView)
{
  Tab* tab = new Tab(tabView);
  tab->text = tab->view->getTabText();
  tab->icon = tab->view->getTabIcon();

  m_list.push_back(tab);

  // Update scroll (in the same position if we can
  setScrollX(m_scrollX);

  startAni(ANI_ADDING_TAB);
}

void Tabs::removeTab(TabView* tabView)
{
  Tab* tab = getTabByView(tabView);
  if (!tab)
    return;

  if (m_hot == tab)
    m_hot = NULL;

  if (m_selected == tab) {
    selectNextTab();
    if (m_selected == tab)
      m_selected = NULL;
  }

  TabsListIterator it =
    std::find(m_list.begin(), m_list.end(), tab);

  ASSERT(it != m_list.end() && "Removing a tab that is not part of the Tabs widget");

  it = m_list.erase(it);

  if (m_removedTab) {
    delete m_removedTab;
    m_removedTab = NULL;
  }
  m_removedTab = tab;

  // Next tab in the list
  if (it != m_list.end())
    m_nextTabOfTheRemovedOne = *it;
  else
    m_nextTabOfTheRemovedOne = NULL;

  // Update scroll (in the same position if we can)
  setScrollX(m_scrollX);

  startAni(ANI_REMOVING_TAB);
}

void Tabs::updateTabsText()
{
  for (Tab* tab : m_list) {
    tab->text = tab->view->getTabText();
    tab->icon = tab->view->getTabIcon();
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
  if (it != m_list.end()) {
    // If we are at the end of the list, cycle to the first tab.
    if (it == --m_list.end())
      it = m_list.begin();
    // Go to next tab.
    else
      ++it;

    if (it != currentTabIt) {
      selectTabInternal(*it);
      if (m_delegate)
        m_delegate->clickTab(this, m_selected->view, kButtonLeft);
    }
  }
}

void Tabs::selectPreviousTab()
{
  TabsListIterator currentTabIt = getTabIteratorByView(m_selected->view);
  TabsListIterator it = currentTabIt;
  if (it != m_list.end()) {
    // If we are at the beginning of the list, cycle to the last tab.
    if (it == m_list.begin())
      it = --m_list.end();
    // Go to previous tab.
    else
      --it;

    if (it != currentTabIt) {
      selectTabInternal(*it);
      if (m_delegate)
        m_delegate->clickTab(this, m_selected->view, kButtonLeft);
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
  switch (msg->type()) {

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
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        if (m_selected != m_hot) {
          m_selected = m_hot;
          invalidate();
        }

        if (m_hotCloseButton) {
          if (!m_clickedCloseButton) {
            m_clickedCloseButton = true;
            invalidate();
          }
        }

        // Left button is processed in mouse down message, right
        // button is processed in mouse up.
        if (m_selected && m_delegate &&
            !m_clickedCloseButton &&
            mouseMsg->left()) {
          m_delegate->clickTab(this, m_selected->view, mouseMsg->buttons());
        }

        captureMouse();
      }
      return true;

    case kMouseUpMessage:
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        if (m_delegate && m_selected && m_selected == m_hot) {
          if (m_hotCloseButton && m_clickedCloseButton) {
            m_clickedCloseButton = false;
            invalidate();

            m_delegate->clickClose(this, m_selected->view);
          }
          else if (!mouseMsg->left()) {
            m_delegate->clickTab(this, m_selected->view, mouseMsg->buttons());
          }
        }
        releaseMouse();
      }
      return true;

    case kMouseWheelMessage: {
      int dz =
        (static_cast<MouseMessage*>(msg)->wheelDelta().x -
         static_cast<MouseMessage*>(msg)->wheelDelta().y) * getBounds().w/6;

      m_begScrollX = m_scrollX;
      if (m_ani != ANI_SMOOTH_SCROLL)
        m_endScrollX = m_scrollX + dz;
      else
        m_endScrollX += dz;

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
              int((m_endScrollX - m_begScrollX) * (1.0-std::exp(-10.0 * m_ani_t / (double)ANI_SMOOTH_SCROLL_TICKS))));
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
  gfx::Rect box(rect.x-m_scrollX, rect.y,
    2*guiscale(),
    (m_list.empty() ? 0:
      theme->dimensions.tabsHeight() - theme->dimensions.tabsEmptyHeight()));

  g->fillRect(theme->colors.windowFace(), g->getClipBounds());

  skin::Style::State state;
  theme->styles.tabFiller()->paint(g, box, nullptr, state);
  theme->styles.tabBottom()->paint(g,
    gfx::Rect(box.x, box.y2(), box.w, rect.y2()-box.y2()), nullptr, state);

  box.x = box.x2();

  // For each tab...
  int tabWidth = calcTabWidth();
  for (Tab* tab : m_list) {
    box.w = tabWidth;

    int x_delta = 0;
    int y_delta = 0;

    // Y-delta for animating tabs (intros and outros)
    if (m_ani == ANI_ADDING_TAB && m_selected == tab) {
      y_delta = box.h * (ANI_ADDING_TAB_TICKS - m_ani_t) / ANI_ADDING_TAB_TICKS;
    }
    else if (m_ani == ANI_REMOVING_TAB && m_nextTabOfTheRemovedOne == tab) {
      x_delta += tabWidth
        - int(double(tabWidth)*(1.0-std::exp(-10.0 * m_ani_t / (double)ANI_REMOVING_TAB_TICKS)));
      x_delta = MID(0, x_delta, tabWidth);

      // Draw deleted tab
      if (m_removedTab) {
        gfx::Rect box2(box.x, box.y, x_delta, box.h);
        drawTab(g, box2, m_removedTab, 0, false, false);
      }
    }

    box.x += x_delta;
    drawTab(g, box, tab, y_delta, (tab == m_hot), (tab == m_selected));

    box.x = box.x2();
  }

  if (m_ani == ANI_REMOVING_TAB && m_nextTabOfTheRemovedOne == NULL) {
    // Draw deleted tab
    if (m_removedTab) {
      int x_delta = tabWidth
        - int(double(tabWidth)*(1.0-std::exp(-10.0 * m_ani_t / (double)ANI_REMOVING_TAB_TICKS)));
      x_delta = MID(0, x_delta, tabWidth);

      gfx::Rect box2(box.x, box.y, x_delta, box.h);
      drawTab(g, box2, m_removedTab, 0, false, false);

      box.x += x_delta;
      box.w = 0;
    }
  }

  // Fill the gap to the right-side
  if (box.x < rect.x2()) {
    theme->styles.tabFiller()->paint(g,
      gfx::Rect(box.x, box.y, rect.x2()-box.x, box.h), nullptr, state);
    theme->styles.tabBottom()->paint(g,
      gfx::Rect(box.x, box.y2(), rect.x2()-box.x, rect.y2()-box.y2()), nullptr, state);
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
  gfx::Size reqsize(0, 0);

  if (m_list.empty())
    reqsize.h = theme->dimensions.tabsEmptyHeight();
  else
    reqsize.h = theme->dimensions.tabsHeight();

  ev.setPreferredSize(reqsize);
}

void Tabs::selectTabInternal(Tab* tab)
{
  m_selected = tab;
  makeTabVisible(tab);
  invalidate();
}

void Tabs::drawTab(Graphics* g, const gfx::Rect& _box, Tab* tab, int dy,
  bool hover, bool selected)
{
  gfx::Rect box = _box;

  // Is the tab outside the bounds of the widget?
  if (box.x >= getBounds().x2() || box.x2() <= getBounds().x)
    return;

  if (box.w < ui::guiscale()*8)
    box.w = ui::guiscale()*8;

  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  gfx::Color text_color;
  gfx::Color face_color;
  int clipTextRightSide;

  gfx::Rect closeBox = getTabCloseButtonBounds(box);
  if (closeBox.isEmpty())
    clipTextRightSide = 4*ui::guiscale();
  else {
    closeBox.y += dy;
    clipTextRightSide = closeBox.w;
  }

  // Selected
  if (selected) {
    text_color = theme->colors.tabActiveText();
    face_color = theme->colors.tabActiveFace();
  }
  // Non-selected
  else {
    text_color = theme->colors.tabNormalText();
    face_color = theme->colors.tabNormalFace();
  }

  skin::Style::State state;
  if (selected) state += skin::Style::active();
  if (hover) state += skin::Style::hover();

  // Tab without text
  theme->styles.tab()->paint(g,
    gfx::Rect(box.x, box.y+dy, box.w, box.h),
    nullptr, state);

  // Tab icon
  TabIcon icon = tab->icon;
  int dx = 0;
  switch (icon) {
    case TabIcon::NONE:
      break;
    case TabIcon::HOME:
      {
        theme->styles.tabHome()->paint(g,
          gfx::Rect(
            box.x,
            box.y+dy,
            box.x-dx,
            box.h),
          nullptr, state);
        dx += theme->dimensions.tabsIconWidth();
      }
      break;
  }

  // Tab with text + clipping the close button
  if (box.w > 8*ui::guiscale()) {
    IntersectClip clip(g, gfx::Rect(box.x+dx, box.y+dy, box.w-dx-clipTextRightSide, box.h));
    theme->styles.tabText()->paint(g,
      gfx::Rect(box.x+dx, box.y+dy, box.w-dx, box.h),
      tab->text.c_str(), state);
  }

  // Tab bottom part
  theme->styles.tabBottom()->paint(g,
    gfx::Rect(box.x, box.y2(), box.w, getBounds().y2()-box.y2()),
    nullptr, state);

  // Close button
  if (!closeBox.isEmpty()) {
    skin::Style* style = theme->styles.tabCloseIcon();
    if (m_delegate &&
      m_delegate->isModified(this, tab->view) &&
      (!hover || !m_hotCloseButton)) {
      style = theme->styles.tabModifiedIcon();
    }

    state = skin::Style::State();
    if (hover && m_hotCloseButton) {
      state += skin::Style::hover();
      if (selected)
        state += skin::Style::active();
      if (m_clickedCloseButton)
        state += skin::Style::clicked();
    }
    else if (selected)
      state += skin::Style::active();

    style->paint(g, closeBox, nullptr, state);
  }
}

Tabs::TabsListIterator Tabs::getTabIteratorByView(TabView* tabView)
{
  TabsListIterator it, end = m_list.end();

  for (it = m_list.begin(); it != end; ++it) {
    if ((*it)->view == tabView)
      break;
  }

  return it;
}

Tabs::Tab* Tabs::getTabByView(TabView* tabView)
{
  TabsListIterator it = getTabIteratorByView(tabView);
  if (it != m_list.end())
    return *it;
  else
    return NULL;
}

int Tabs::getMaxScrollX()
{
  TabsListIterator it, end = m_list.end();
  int x = 0;
  int tabWidth = calcTabWidth();

  for (it = m_list.begin(); it != end; ++it) {
    Tab* tab = *it;
    x += tabWidth;
  }

  x -= getBounds().w;

  if (x < 0)
    return 0;
  else
    return x + ARROW_W*2;
}

void Tabs::makeTabVisible(Tab* make_visible_this_tab)
{
  int x = 0;
  int extra_x = getMaxScrollX() > 0 ? ARROW_W*2: 0;
  int tabWidth = calcTabWidth();

  for (Tab* tab : m_list) {
    if (tab == make_visible_this_tab) {
      if (x - m_scrollX < 0) {
        setScrollX(x);
      }
      else if (x + tabWidth - m_scrollX > getBounds().w - extra_x) {
        setScrollX(x + tabWidth - getBounds().w + extra_x);
      }
      break;
    }

    x += tabWidth;
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
}

void Tabs::calculateHot()
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  gfx::Rect rect = getBounds();
  gfx::Rect box(rect.x-m_scrollX, rect.y, 0, rect.h-1);
  gfx::Point mousePos = ui::get_mouse_position();
  Tab* hot = NULL;
  bool hotCloseButton = false;
  int tabWidth = calcTabWidth();

  // For each tab
  for (Tab* tab : m_list) {
    box.w = tabWidth;

    if (box.contains(mousePos)) {
      hot = tab;
      hotCloseButton = getTabCloseButtonBounds(box).contains(mousePos);
      break;
    }

    box.x += box.w;
  }

  if (m_hot != hot ||
      m_hotCloseButton != hotCloseButton) {
    m_hot = hot;
    m_hotCloseButton = hotCloseButton;

    if (m_delegate)
      m_delegate->mouseOverTab(this, m_hot ? m_hot->view: NULL);

    invalidate();
  }
}

int Tabs::calcTabWidth()
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  int tabWidth = theme->dimensions.tabsWidth();

  if (tabWidth * m_list.size() > (size_t)getBounds().w) {
    tabWidth = getBounds().w / m_list.size();
    tabWidth = MAX(2*ui::guiscale(), tabWidth);
  }

  return tabWidth;
}

gfx::Rect Tabs::getTabCloseButtonBounds(const gfx::Rect& box)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  int iconW = theme->dimensions.tabsCloseIconWidth();
  int iconH = theme->dimensions.tabsCloseIconHeight();

  if (box.w-iconW > 4*ui::guiscale())
    return gfx::Rect(box.x2()-iconW, box.y+box.h/2-iconH/2, iconW, iconH);
  else
    return gfx::Rect();
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

} // namespace app
