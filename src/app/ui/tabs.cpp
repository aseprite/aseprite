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
#define ANI_REORDER_TABS_TICKS    5

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
  , m_hot(nullptr)
  , m_hotCloseButton(false)
  , m_clickedCloseButton(false)
  , m_selected(nullptr)
  , m_delegate(delegate)
  , m_timer(1000/60, this)
  , m_ani(ANI_NONE)
  , m_removedTab(nullptr)
  , m_isDragging(false)
{
  setDoubleBuffered(true);
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

void Tabs::addTab(TabView* tabView, int pos)
{
  resetOldPositions();
  startAni(ANI_ADDING_TAB, ANI_ADDING_TAB_TICKS);

  Tab* tab = new Tab(tabView);
  if (pos < 0)
    m_list.push_back(tab);
  else
    m_list.insert(m_list.begin()+pos, tab);
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
    if (tab == m_list.back())
      selectPreviousTab();
    else
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
    tab->modified = m_delegate->onIsModified(this, tabView);
  tab->view = nullptr;          // The view will be destroyed after Tabs::removeTab() anyway

  resetOldPositions();
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
  int i = 0;

  for (Tab* tab : m_list) {
    tab->text = tab->view->getTabText();
    tab->icon = tab->view->getTabIcon();
    tab->x = int(x);
    tab->width = int(x+tabWidth) - int(x);
    x += tabWidth;
    ++i;
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

    if (it != currentTabIt)
      selectTabInternal(*it);
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

    if (it != currentTabIt)
      selectTabInternal(*it);
  }
}

TabView* Tabs::getSelectedTab()
{
  if (m_selected)
    return m_selected->view;
  else
    return NULL;
}

bool Tabs::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseEnterMessage:
      calculateHot();
      return true;

    case kMouseMoveMessage:
      calculateHot();

      if (hasCapture() && m_selected) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        gfx::Point mousePos = mouseMsg->position();
        gfx::Point delta = mousePos - m_dragMousePos;

        if (!m_isDragging) {
          if (!m_clickedCloseButton) {
            double dist = std::sqrt(delta.x*delta.x + delta.y*delta.y);
            if (dist > 4.0/ui::guiscale())
              startDrag();
          }
        }
        // We are drag a tab...
        else {
          m_selected->x = m_dragTabX + delta.x;

          int i = (mousePos.x-m_border*guiscale()) / m_selected->width;
          i = MID(0, i, int(m_list.size())-1);
          if (i != m_dragTabIndex) {
            std::swap(m_list[m_dragTabIndex], m_list[i]);
            m_dragTabIndex = i;

            resetOldPositions(double(m_ani_t) / double(m_ani_T));
            updateTabs();
            startAni(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);
          }

          invalidate();
        }
      }
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
        m_dragMousePos = mouseMsg->position();

        if (m_hotCloseButton) {
          if (!m_clickedCloseButton) {
            m_clickedCloseButton = true;
            invalidate();
          }
        }
        else if (mouseMsg->left() && m_selected != m_hot) {
          m_selected = m_hot;

          // Left-click is processed in mouse down message,
          // right-click is processed in mouse up.
          if (m_selected && m_delegate)
            m_delegate->onSelectTab(this, m_selected->view);

          invalidate();
        }

        captureMouse();
      }
      return true;

    case kMouseUpMessage:
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

        if ((mouseMsg->middle()) ||
            (mouseMsg->left() && m_hotCloseButton && m_clickedCloseButton)) {
          if (m_hot && m_delegate)
            m_delegate->onCloseTab(this, m_hot->view);
        }
        else if (mouseMsg->right() && m_hot) {
          if (m_delegate)
            m_delegate->onContextMenuTab(this, m_hot->view);
        }

        releaseMouse();

        if (m_isDragging)
          stopDrag();

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
        if (newIndex != index) {
          selectTabInternal(m_list[newIndex]);
        }
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
  gfx::Rect box(rect.x, rect.y, rect.w,
    (m_list.empty() && m_ani == ANI_NONE ? 0:
      theme->dimensions.tabsHeight() - theme->dimensions.tabsEmptyHeight()));

  g->fillRect(theme->colors.windowFace(), g->getClipBounds());
  drawFiller(g, box);

  int startX = m_border*guiscale();
  double t = double(m_ani_t)/double(m_ani_T);

  // For each tab...
  int i = 0;
  for (Tab* tab : m_list) {
    if (m_ani == ANI_NONE) {
      box.x = startX + tab->x;
      box.w = tab->width;
    }
    else {
      box.x = startX + int(inbetween(tab->oldX, tab->x, t));
      box.w = int(inbetween(tab->oldWidth, tab->width, t));
    }

    if (tab != m_selected)
      drawTab(g, box, tab, 0, (tab == m_hot), false);

    box.x = box.x2();
    ++i;
  }

  // Draw deleted tab
  if (m_ani == ANI_REMOVING_TAB && m_removedTab) {
    box.x = startX + m_removedTab->x;
    box.w = int(inbetween(m_removedTab->oldWidth, 0, t));
    drawTab(g, box, m_removedTab, 0, false, false);
  }

  // Tab that is being dragged
  if (m_selected) {
    Tab* tab = m_selected;

    if (m_ani == ANI_NONE) {
      box.x = startX + tab->x;
      box.w = tab->width;
    }
    else {
      box.x = startX + int(inbetween(tab->oldX, tab->x, t));
      box.w = int(inbetween(tab->oldWidth, tab->width, t));
    }

    int dy = 0;
    if (m_ani == ANI_ADDING_TAB)
      dy = int(box.h - box.h * t);

    drawTab(g, box, m_selected, dy, (tab == m_hot), true);
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

  if (m_list.empty() && m_ani == ANI_NONE)
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

  if (m_delegate)
    m_delegate->onSelectTab(this, tab->view);
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
  int clipTextRightSide;

  gfx::Rect closeBox = getTabCloseButtonBounds(tab, box);
  if (closeBox.isEmpty())
    clipTextRightSide = 4*ui::guiscale();
  else {
    closeBox.y += dy;
    clipTextRightSide = closeBox.w;
  }

  skin::Style::State state;
  if (selected) state += skin::Style::active();
  if (hover) state += skin::Style::hover();

  // Tab without text
  theme->styles.tab()->paint(g,
    gfx::Rect(box.x, box.y+dy, box.w, box.h),
    nullptr, state);

  {
    IntersectClip clip(g, gfx::Rect(box.x, box.y+dy, box.w-clipTextRightSide, box.h));

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
      theme->styles.tabText()->paint(g,
        gfx::Rect(box.x+dx, box.y+dy, box.w-dx, box.h),
        tab->text.c_str(), state);
    }
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
        tab->modified = m_delegate->onIsModified(this, tab->view);

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

void Tabs::drawFiller(ui::Graphics* g, const gfx::Rect& box)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
  gfx::Rect rect = getClientBounds();
  skin::Style::State state;

  theme->styles.tabFiller()->paint(g,
    gfx::Rect(box.x, box.y, rect.x2()-box.x, box.h), nullptr, state);

  theme->styles.tabBottom()->paint(g,
    gfx::Rect(box.x, box.y2(), rect.x2()-box.x, rect.y2()-box.y2()), nullptr, state);
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
  if (m_isDragging)
    return;

  gfx::Rect rect = getBounds();
  gfx::Rect box(rect.x+m_border*guiscale(), rect.y, 0, rect.h-1);
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
      m_delegate->onMouseOverTab(this, m_hot ? m_hot->view: NULL);

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

void Tabs::resetOldPositions()
{
  for (Tab* tab : m_list) {
    tab->oldX = tab->x;
    tab->oldWidth = tab->width;
  }
}

void Tabs::resetOldPositions(double t)
{
  for (Tab* tab : m_list) {
    tab->oldX = int(inbetween(tab->oldX, tab->x, t));
    tab->oldWidth = int(inbetween(tab->oldWidth, tab->width, t));
  }
}

void Tabs::startAni(Ani ani, int T)
{
  // Stop previous animation
  if (m_ani != ANI_NONE)
    stopAni();

  m_ani = ani;
  m_ani_t = 0;
  m_ani_T = T;
  m_timer.start();
}

void Tabs::stopAni()
{
  m_ani = ANI_NONE;
  m_timer.stop();

  if (m_list.empty()) {
    Widget* root = getRoot();
    if (root)
      root->layout();
  }
}

void Tabs::startDrag()
{
  ASSERT(m_selected);

  updateTabs();

  m_isDragging = true;
  m_dragTabX = m_selected->x;
  m_dragTabIndex = std::find(m_list.begin(), m_list.end(), m_selected) - m_list.begin();
}

void Tabs::stopDrag()
{
  m_isDragging = false;

  if (m_selected) {
    m_selected->oldX = m_selected->x;
    m_selected->oldWidth = m_selected->width;
  }

  resetOldPositions(double(m_ani_t) / double(m_ani_T));
  updateTabs();
  startAni(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);
}

} // namespace app
