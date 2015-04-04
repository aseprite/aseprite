// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/tabs.h"

#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "she/font.h"
#include "she/scoped_surface_lock.h"
#include "she/surface.h"
#include "she/system.h"
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

WidgetType Tabs::Type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

Tabs::Tabs(TabsDelegate* delegate)
  : Widget(Tabs::Type())
  , m_border(2)
  , m_hot(nullptr)
  , m_hotCloseButton(false)
  , m_clickedCloseButton(false)
  , m_selected(nullptr)
  , m_docked(false)
  , m_delegate(delegate)
  , m_removedTab(nullptr)
  , m_isDragging(false)
  , m_floatingTab(nullptr)
  , m_floatingOverlay(nullptr)
  , m_dropNewTab(nullptr)
  , m_dropNewIndex(-1)
{
  setDoubleBuffered(true);
  initTheme();

  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  m_tabsHeight = theme->dimensions.tabsHeight();
  m_tabsEmptyHeight = theme->dimensions.tabsEmptyHeight();
  setBgColor(theme->colors.windowFace());
}

Tabs::~Tabs()
{
  m_removedTab.reset();

  // Stop animation
  stopAnimation();

  // Remove all tabs
  m_list.clear();
}

void Tabs::addTab(TabView* tabView, bool from_drop, int pos)
{
  resetOldPositions();
  if (!from_drop)
    startAnimation(ANI_ADDING_TAB, ANI_ADDING_TAB_TICKS);
  else
    startAnimation(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);

  TabPtr tab(new Tab(tabView));
  if (pos < 0)
    m_list.push_back(tab);
  else
    m_list.insert(m_list.begin()+pos, tab);
  updateTabs();

  tab->oldX = (from_drop ? m_dropNewPosX-tab->width/2: tab->x);
  tab->oldWidth = tab->width;
  tab->modified = (m_delegate ? m_delegate->onIsModified(this, tabView): false);
}

void Tabs::removeTab(TabView* tabView, bool with_animation)
{
  TabPtr tab(getTabByView(tabView));
  if (!tab)
    return;

  if (m_hot == tab)
    m_hot.reset();

  if (m_selected == tab) {
    if (tab == m_list.back())
      selectPreviousTab();
    else
      selectNextTab();

    if (m_selected == tab)
      m_selected.reset();
  }

  TabsListIterator it =
    std::find(m_list.begin(), m_list.end(), tab);
  ASSERT(it != m_list.end() && "Removing a tab that is not part of the Tabs widget");
  it = m_list.erase(it);

  m_removedTab = tab;

  if (with_animation) {
    if (m_delegate)
      tab->modified = m_delegate->onIsModified(this, tabView);
    tab->view = nullptr;          // The view will be destroyed after Tabs::removeTab() anyway

    resetOldPositions();
    startAnimation(ANI_REMOVING_TAB, ANI_REMOVING_TAB_TICKS);
  }

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

  for (auto& tab : m_list) {
    if (tab == m_floatingTab) {
      ++i;
      continue;
    }

    if (m_dropNewTab && m_dropNewIndex == i)
      x += tabWidth;

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

  TabPtr tab(getTabByView(tabView));
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

void Tabs::setDockedStyle()
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());

  m_docked = true;
  m_tabsHeight = theme->dimensions.dockedTabsHeight();
  m_tabsEmptyHeight = 0;

  setBgColor(theme->colors.workspace());
}

void Tabs::setDropViewPreview(const gfx::Point& pos, TabView* view)
{
  int newIndex = -1;

  if (!m_list.empty()) {
    newIndex = (pos.x - getBounds().x) / m_list[0]->width;
    newIndex = MID(0, newIndex, (int)m_list.size());
  }
  else
    newIndex = 0;

  bool startAni = (m_dropNewIndex != newIndex || m_dropNewTab != view);

  m_dropNewIndex = newIndex;
  m_dropNewPosX = (pos.x - getBounds().x);
  m_dropNewTab = view;

  if (startAni) {
    resetOldPositions(animationTime());
    updateTabs();
    startAnimation(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);
  }
  else
    invalidate();
}

void Tabs::removeDropViewPreview()
{
  m_dropNewTab = nullptr;

  resetOldPositions(animationTime());
  updateTabs();
  startAnimation(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);
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
          if (!m_clickedCloseButton && mouseMsg->left()) {
            double dist = std::sqrt(delta.x*delta.x + delta.y*delta.y);
            if (dist > 4.0/ui::guiscale())
              startDrag();
          }
        }
        // We are drag a tab...
        else {
          TabPtr justDocked(nullptr);
          bool dockedInThisTabs = true;

          // Floating tab (to create a new window)
          if (!getBounds().contains(mousePos) &&
              (ABS(delta.y) > 16*guiscale() ||
                mousePos.x < getBounds().x-16*guiscale() ||
                mousePos.x > getBounds().x2()+16*guiscale())) {
            DropViewPreviewResult result = DropViewPreviewResult::FLOATING;

            if (m_delegate)
              result = m_delegate->onFloatingTab(this, m_selected->view, mousePos);

            if (result != DropViewPreviewResult::DROP_IN_TABS) {
              if (!m_floatingOverlay)
                createFloatingTab(m_selected);
              m_floatingOverlay->moveOverlay(mousePos - m_dragOffset);
              dockedInThisTabs = false;
            }
            else {
              destroyFloatingOverlay();
            }
          }
          else {
            justDocked = m_floatingTab;
            destroyFloatingTab();

            if (m_delegate)
              m_delegate->onDockingTab(this, m_selected->view);
          }

          // Docked tab
          if (dockedInThisTabs) {
            m_selected->x = m_dragTabX + delta.x;

            int i = (mousePos.x - m_border*guiscale() - getBounds().x) / m_selected->width;
            i = MID(0, i, int(m_list.size())-1);
            if (i != m_dragTabIndex) {
              m_list.erase(m_list.begin()+m_dragTabIndex);
              m_list.insert(m_list.begin()+i, m_selected);
              m_dragTabIndex = i;

              resetOldPositions(animationTime());
              updateTabs();
              startAnimation(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);
            }

            if (justDocked)
              justDocked->oldX = m_dragTabX + delta.x;
          }

          invalidate();
        }
      }
      return true;

    case kMouseLeaveMessage:
      if (m_hot) {
        m_hot.reset();
        invalidate();
      }
      return true;

    case kMouseDownMessage:
      if (m_hot && !hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        m_dragMousePos = mouseMsg->position();
        m_dragOffset = mouseMsg->position() -
          (getBounds().getOrigin() + getTabBounds(m_hot.get()).getOrigin());

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

        releaseMouse();

        if (!m_isDragging) {
          if ((mouseMsg->middle()) ||
              (mouseMsg->left() && m_hotCloseButton && m_clickedCloseButton)) {
            if (m_hot && m_delegate)
              m_delegate->onCloseTab(this, m_hot->view);
          }
          else if (mouseMsg->right() && m_hot) {
            if (m_delegate)
              m_delegate->onContextMenuTab(this, m_hot->view);
          }

          if (m_clickedCloseButton) {
            m_clickedCloseButton = false;
            invalidate();
          }
        }
        else {
          DropTabResult result = DropTabResult::IGNORE;

          if (m_delegate) {
            ASSERT(m_selected);
            result =
              m_delegate->onDropTab(this, m_selected->view,
                mouseMsg->position());
          }

          stopDrag(result);
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

  }

  return Widget::onProcessMessage(msg);
}

void Tabs::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  gfx::Rect rect = getClientBounds();
  gfx::Rect box(rect.x, rect.y, rect.w,
    (m_list.empty() && animation() == ANI_NONE ? 0:
      m_tabsHeight - m_tabsEmptyHeight));

  g->fillRect(getBgColor(), g->getClipBounds());

  if (!m_docked)
    drawFiller(g, box);

  // For each tab...
  for (TabPtr& tab : m_list) {
    if (tab == m_floatingTab)
      continue;

    box = getTabBounds(tab.get());

    if (tab != m_selected)
      drawTab(g, box, tab.get(), 0, (tab == m_hot), false);

    box.x = box.x2();
  }

  // Draw deleted tab
  if (animation() == ANI_REMOVING_TAB && m_removedTab) {
    m_removedTab->width = 0;
    box = getTabBounds(m_removedTab.get());
    drawTab(g, box, m_removedTab.get(), 0,
      (m_removedTab == m_floatingTab),
      (m_removedTab == m_floatingTab));
  }

  // Tab that is being dragged
  if (m_selected && m_selected != m_floatingTab) {
    double t = animationTime();
    TabPtr tab(m_selected);
    box = getTabBounds(tab.get());

    int dy = 0;
    if (animation() == ANI_ADDING_TAB)
      dy = int(box.h - box.h * t);

    drawTab(g, box, m_selected.get(), dy, (tab == m_hot), true);
  }

  // New tab from other Tab that want to be dropped here.
  if (m_dropNewTab) {
    SkinTheme* theme = static_cast<SkinTheme*>(this->getTheme());
    Tab newTab(m_dropNewTab);
    newTab.text = m_dropNewTab->getTabText();
    newTab.icon = m_dropNewTab->getTabIcon();
    newTab.width = (!m_list.empty() ? m_list[0]->width:
                    theme->dimensions.tabsWidth());
    newTab.x = m_dropNewPosX - newTab.width/2;
    box = getTabBounds(&newTab);
    drawTab(g, box, &newTab, 0, true, true);
  }
}

void Tabs::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.getBounds());
  updateTabs();
}

void Tabs::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Size reqsize(0, 0);

  if (m_list.empty() && animation() == ANI_NONE)
    reqsize.h = m_tabsEmptyHeight;
  else
    reqsize.h = m_tabsHeight;

  ev.setPreferredSize(reqsize);
}

void Tabs::selectTabInternal(TabPtr& tab)
{
  m_selected = tab;
  makeTabVisible(tab.get());
  invalidate();

  if (m_delegate)
    m_delegate->onSelectTab(this, tab->view);
}

void Tabs::drawTab(Graphics* g, const gfx::Rect& _box, Tab* tab, int dy,
  bool hover, bool selected)
{
  gfx::Rect box = _box;
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
  if (!m_docked)
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

Tabs::TabPtr Tabs::getTabByView(TabView* tabView)
{
  TabsListIterator it = getTabIteratorByView(tabView);
  if (it != m_list.end())
    return TabPtr(*it);
  else
    return TabPtr(nullptr);
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
  TabPtr hot(nullptr);
  bool hotCloseButton = false;

  // For each tab
  for (TabPtr& tab : m_list) {
    if (tab == m_floatingTab)
      continue;

    box.w = tab->width;

    if (box.contains(mousePos)) {
      hot = tab;
      hotCloseButton = getTabCloseButtonBounds(tab.get(), box).contains(mousePos);
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

  if (box.w-iconW > 32*ui::guiscale() || tab == m_selected.get())
    return gfx::Rect(box.x2()-iconW, box.y+box.h/2-iconH/2, iconW, iconH);
  else
    return gfx::Rect();
}

void Tabs::resetOldPositions()
{
  for (TabPtr& tab : m_list) {
    if (tab == m_floatingTab)
      continue;

    tab->oldX = tab->x;
    tab->oldWidth = tab->width;
  }
}

void Tabs::resetOldPositions(double t)
{
  for (TabPtr& tab : m_list) {
    if (tab == m_floatingTab)
      continue;

    tab->oldX = int(inbetween(tab->oldX, tab->x, t));
    tab->oldWidth = int(inbetween(tab->oldWidth, tab->width, t));
  }
}

void Tabs::onAnimationFrame()
{
  invalidate();
}

void Tabs::onAnimationStop(int animation)
{
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

void Tabs::stopDrag(DropTabResult result)
{
  m_isDragging = false;

  switch (result) {

    case DropTabResult::IGNORE:
      destroyFloatingTab();

      ASSERT(m_selected);
      if (m_selected) {
        m_selected->oldX = m_selected->x;
        m_selected->oldWidth = m_selected->width;
      }
      resetOldPositions(animationTime());
      updateTabs();
      startAnimation(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);
      break;

    case DropTabResult::DOCKED_IN_OTHER_PLACE: {
      TabPtr tab = m_floatingTab;

      m_floatingTab.reset();
      m_removedTab.reset();
      destroyFloatingTab();

      ASSERT(tab.get());
      if (tab)
        removeTab(tab->view, false);
      break;
    }

  }
}

gfx::Rect Tabs::getTabBounds(Tab* tab)
{
  gfx::Rect rect = getClientBounds();
  gfx::Rect box(rect.x, rect.y, rect.w,
    (m_list.empty() && animation() == ANI_NONE ? 0:
      m_tabsHeight - m_tabsEmptyHeight));
  int startX = m_border*guiscale();
  double t = animationTime();

  if (animation() == ANI_NONE) {
    box.x = startX + tab->x;
    box.w = tab->width;
  }
  else {
    box.x = startX + int(inbetween(tab->oldX, tab->x, t));
    box.w = int(inbetween(tab->oldWidth, tab->width, t));
  }

  return box;
}

void Tabs::createFloatingTab(TabPtr& tab)
{
  ASSERT(!m_floatingOverlay);

  she::Surface* surface = she::instance()->createRgbaSurface(
    tab->width, m_tabsHeight);

  {
    Graphics g(surface, 0, 0);
    g.setFont(getFont());
    g.fillRect(gfx::ColorNone, g.getClipBounds());
    drawTab(&g, g.getClipBounds(), tab.get(), 0, true, true);
  }

  // Make opaque (TODO this shouldn't be necessary)
  {
    she::ScopedSurfaceLock lock(surface);
    gfx::Color mask = lock->getPixel(0, 0);

    for (int y=0; y<surface->height(); ++y)
      for (int x=0; x<surface->width(); ++x) {
        gfx::Color c = lock->getPixel(x, y);
        c = (c != mask ? gfx::seta(c, 255): gfx::ColorNone);
        lock->putPixel(c, x, y);
      }
  }

  m_floatingOverlay.reset(new Overlay(surface, gfx::Point(), Overlay::MouseZOrder-1));
  OverlayManager::instance()->addOverlay(m_floatingOverlay.get());

  resetOldPositions();

  m_floatingTab = tab;
  m_removedTab.reset();
  startAnimation(ANI_REMOVING_TAB, ANI_REMOVING_TAB_TICKS);
  updateTabs();
}

void Tabs::destroyFloatingTab()
{
  destroyFloatingOverlay();

  if (m_floatingTab) {
    TabPtr tab(m_floatingTab);
    m_floatingTab.reset();

    resetOldPositions();
    startAnimation(ANI_ADDING_TAB, ANI_ADDING_TAB_TICKS);
    updateTabs();

    tab->oldX = tab->x;
    tab->oldWidth = 0;
  }
}

void Tabs::destroyFloatingOverlay()
{
  if (m_floatingOverlay) {
    OverlayManager::instance()->removeOverlay(m_floatingOverlay.get());
    m_floatingOverlay.reset();
  }
}

} // namespace app
