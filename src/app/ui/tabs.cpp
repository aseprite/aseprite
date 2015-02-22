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

#define ANI_ADDING_TAB_TICKS      5
#define ANI_REMOVING_TAB_TICKS    10

#define HAS_ARROWS(tabs) ((m_button_left->getParent() == (tabs)))

namespace app {

using namespace app::skin;
using namespace ui;

namespace {
  double ease(double t) {
    return (1.0 - std::pow(1.0 - t, 2));
  }
  double inbetween(double x0, double x1, double t) {
    return x0 + (x1-x0)*ease(t);
  }
}

static WidgetType tabs_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

Tabs::Tabs(TabsDelegate* delegate)
  : Widget(tabs_type())
  , m_border(2)
  , m_delegate(delegate)
  , m_timer(1000/60, this)
{
  setDoubleBuffered(true);

  m_hot = NULL;
  m_hotCloseButton = false;
  m_selected = NULL;
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
  startAni(ANI_ADDING_TAB, ANI_ADDING_TAB_TICKS);

  Tab* tab = new Tab(tabView);
  m_list.push_back(tab);
  updateTabs();

  tab->oldX = tab->x;
  tab->oldWidth = tab->width;
  tab->modified = false;
}

void Tabs::removeTab(TabView* tabView)
{
  Tab* tab = getTabByView(tabView);
  if (!tab)
    return;

  if (m_hot == tab)
    m_hot = nullptr;

  if (m_selected == tab) {
    selectNextTab();
    if (m_selected == tab)
      m_selected = nullptr;
  }

  TabsListIterator it =
    std::find(m_list.begin(), m_list.end(), tab);
  ASSERT(it != m_list.end() && "Removing a tab that is not part of the Tabs widget");
  it = m_list.erase(it);

  delete m_removedTab;
  m_removedTab = tab;

  if (m_delegate)
    tab->modified = m_delegate->isModified(this, tabView);
  tab->view = nullptr;          // The view will be destroyed after Tabs::removeTab() anyway

  // Next tab in the list
  if (it != m_list.end())
    m_nextTabOfTheRemovedOne = *it;
  else
    m_nextTabOfTheRemovedOne = nullptr;

  startAni(ANI_REMOVING_TAB, ANI_REMOVING_TAB_TICKS);
  updateTabs();
}

void Tabs::updateTabs()
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  double availWidth = getBounds().w - m_border*ui::guiscale();
  double defTabWidth = theme->dimensions.tabsWidth();
  double tabWidth = defTabWidth;
  if (tabWidth * m_list.size() > availWidth) {
    tabWidth = availWidth / double(m_list.size());
    tabWidth = MAX(4*ui::guiscale(), tabWidth);
  }
  double x = 0.0;

  for (Tab* tab : m_list) {
    double thisTabWidth;

    // if (tab == m_selected)
    //   thisTabWidth = defTabWidth;
    // else
      thisTabWidth = tabWidth;

    tab->text = tab->view->getTabText();
    tab->icon = tab->view->getTabIcon();
    tab->x = int(x);
    tab->width = int(x+thisTabWidth) - int(x);
    x += thisTabWidth;
  }
  invalidate();
}

void Tabs::selectTab(TabView* tabView)
{
  ASSERT(tabView != NULL);

  Tab* tab = getTabByView(tabView);
  if (tab)
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
            m_delegate->clickClose(this, m_selected->view);
          }
          else if (!mouseMsg->left()) {
            m_delegate->clickTab(this, m_selected->view, mouseMsg->buttons());
          }
        }

        releaseMouse();

        if (m_clickedCloseButton) {
          m_clickedCloseButton = false;
          invalidate();
        }
      }
      return true;

    case kMouseWheelMessage: {
      int dz =
        (static_cast<MouseMessage*>(msg)->wheelDelta().x +
         static_cast<MouseMessage*>(msg)->wheelDelta().y);

      auto it = std::find(m_list.begin(), m_list.end(), m_selected);
      if (it != m_list.end()) {
        int index = (it - m_list.begin());
        int newIndex = index + dz;
        newIndex = MID(0, newIndex, int(m_list.size())-1);
        if (newIndex != index)
          selectTabInternal(m_list[newIndex]);
      }
      return true;
    }

    case kTimerMessage: {
      if (m_ani != ANI_NONE) {
        if (m_ani_t == m_ani_T)
          stopAni();
        else
          ++m_ani_t;

        invalidate();
      }
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
  gfx::Rect box(rect.x, rect.y,
    m_border*guiscale(),
    (m_list.empty() ? 0:
      theme->dimensions.tabsHeight() - theme->dimensions.tabsEmptyHeight()));

  g->fillRect(theme->colors.windowFace(), g->getClipBounds());

  skin::Style::State state;
  theme->styles.tabFiller()->paint(g, box, nullptr, state);
  theme->styles.tabBottom()->paint(g,
    gfx::Rect(box.x, box.y2(), box.w, rect.y2()-box.y2()), nullptr, state);

  box.x = box.x2();
  int startX = box.x;
  double t = double(m_ani_t)/double(m_ani_T);
  Tab* prevTab = nullptr;

  // For each tab...
  for (Tab* tab : m_list) {
    int prevX2 = box.x;

    if (m_ani == ANI_NONE) {
      box.w = tab->width;
    }
    else {
      box.x = startX + int(inbetween(tab->oldX, tab->x, t));
      box.w = int(inbetween(tab->oldWidth, tab->width, t));
    }

    int dy = 0;
    if (m_ani == ANI_ADDING_TAB) {
      box.x = prevX2;            // To avoid empty spaces in animation between tabs
      if (m_selected == tab)
        dy = int(box.h - box.h * t);
    }
    else if (m_ani == ANI_REMOVING_TAB) {
      if (m_nextTabOfTheRemovedOne == tab) {
        // Draw deleted tab
        if (m_removedTab) {
          gfx::Rect box2(0, box.y, 0, box.h);
          if (prevTab)
            box2.x = prevX2;
          box2.w = int(startX + inbetween(tab->oldX, tab->x, t)) - box2.x;
          drawTab(g, box2, m_removedTab, 0, false, false);
        }
      }
      else
        box.x = prevX2;            // To avoid empty spaces in animation between tabs
    }

    drawTab(g, box, tab, dy, (tab == m_hot), (tab == m_selected));

    box.x = box.x2();
    prevTab = tab;
  }

  // Draw deleted tab
  if (m_ani == ANI_REMOVING_TAB && !m_nextTabOfTheRemovedOne && m_removedTab) {
    gfx::Rect box2(0, box.y, 0, box.h);
    if (prevTab) {
      box2.x = int(startX + inbetween(
          prevTab->oldX+prevTab->oldWidth, prevTab->x+prevTab->width, t));
    }
    box2.w = int(inbetween(m_removedTab->oldWidth, 0, t));
    drawTab(g, box2, m_removedTab, 0, false, false);

    box.x += box2.w;
    box.w = 0;
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
  updateTabs();
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

  gfx::Rect closeBox = getTabCloseButtonBounds(tab, box);
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

    if (m_delegate) {
      if (tab->view)
        tab->modified = m_delegate->isModified(this, tab->view);

      if (tab->modified &&
          (!hover || !m_hotCloseButton)) {
        style = theme->styles.tabModifiedIcon();
      }
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

void Tabs::makeTabVisible(Tab* thisTab)
{
  updateTabs();
}

void Tabs::calculateHot()
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  gfx::Rect rect = getBounds();
  gfx::Rect box(rect.x, rect.y, 0, rect.h-1);
  gfx::Point mousePos = ui::get_mouse_position();
  Tab* hot = NULL;
  bool hotCloseButton = false;

  // For each tab
  for (Tab* tab : m_list) {
    box.w = tab->width;

    if (box.contains(mousePos)) {
      hot = tab;
      hotCloseButton = getTabCloseButtonBounds(tab, box).contains(mousePos);
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

gfx::Rect Tabs::getTabCloseButtonBounds(Tab* tab, const gfx::Rect& box)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  int iconW = theme->dimensions.tabsCloseIconWidth();
  int iconH = theme->dimensions.tabsCloseIconHeight();

  if (box.w-iconW > 32*ui::guiscale() || tab == m_selected)
    return gfx::Rect(box.x2()-iconW, box.y+box.h/2-iconH/2, iconW, iconH);
  else
    return gfx::Rect();
}

void Tabs::startAni(Ani ani, int T)
{
  // Stop previous animation
  if (m_ani != ANI_NONE)
    stopAni();

  for (Tab* tab : m_list) {
    tab->oldX = tab->x;
    tab->oldWidth = tab->width;
  }

  m_ani = ani;
  m_ani_t = 0;
  m_ani_T = T;
  m_timer.start();
}

void Tabs::stopAni()
{
  m_ani = ANI_NONE;
  m_timer.stop();
}

} // namespace app
