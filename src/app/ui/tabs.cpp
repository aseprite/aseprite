// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/tabs.h"

#include "app/color_utils.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor_view.h"
#include "app/ui/skin/skin_theme.h"
#include "os/font.h"
#include "os/surface.h"
#include "os/system.h"
#include "ui/intern.h"
#include "ui/ui.h"

#include <algorithm>
#include <cmath>

#define ANI_ADDING_TAB_TICKS      5
#define ANI_REMOVING_TAB_TICKS    10
#define ANI_REORDER_TABS_TICKS    5

#define HAS_ARROWS(tabs) ((m_button_left->parent() == (tabs)))

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
  , m_docked(false)
  , m_hot(nullptr)
  , m_hotCloseButton(false)
  , m_clickedCloseButton(false)
  , m_selected(nullptr)
  , m_delegate(delegate)
  , m_addedTab(nullptr)
  , m_removedTab(nullptr)
  , m_isDragging(false)
  , m_dragCopy(false)
  , m_dragTab(nullptr)
  , m_floatingTab(nullptr)
  , m_floatingOverlay(nullptr)
  , m_dropNewTab(nullptr)
  , m_dropNewIndex(-1)
{
  enableFlags(CTRL_RIGHT_CLICK);
  setDoubleBuffered(true);
  initTheme();
}

Tabs::~Tabs()
{
  m_addedTab.reset();
  m_removedTab.reset();

  // Stop animation
  stopAnimation();

  // Remove all tabs
  m_list.clear();
}

void Tabs::addTab(TabView* tabView, bool from_drop, int pos)
{
  resetOldPositions();
  if (!from_drop || m_list.empty())
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
  if (tab->oldX < 0)
    tab->oldX = 0;

  tab->oldWidth = tab->width;
  tab->modified = (m_delegate ? m_delegate->isTabModified(this, tabView): false);

  m_addedTab = tab;
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

  if (with_animation)
    resetOldPositions();

  TabsListIterator it =
    std::find(m_list.begin(), m_list.end(), tab);
  ASSERT(it != m_list.end() && "Removing a tab that is not part of the Tabs widget");
  it = m_list.erase(it);

  m_removedTab = tab;

  if (with_animation) {
    if (m_delegate)
      tab->modified = m_delegate->isTabModified(this, tabView);
    tab->view = nullptr;          // The view will be destroyed after Tabs::removeTab() anyway

    startAnimation(ANI_REMOVING_TAB, ANI_REMOVING_TAB_TICKS);
  }

  updateTabs();
}

void Tabs::updateTabs()
{
  auto theme = SkinTheme::get(this);
  double availWidth = bounds().w - m_border*ui::guiscale();
  double defTabWidth = theme->dimensions.tabsWidth();
  double tabWidth = defTabWidth;
  if (tabWidth * m_list.size() > availWidth) {
    tabWidth = availWidth / double(m_list.size());
    tabWidth = std::max(double(4*ui::guiscale()), tabWidth);
  }
  double x = 0.0;
  int i = 0;

  for (auto& tab : m_list) {
    if (tab == m_floatingTab && !m_dragCopy) {
      ++i;
      continue;
    }

    if ((m_dropNewTab && m_dropNewIndex == i) ||
        (m_dragTab && !m_floatingTab &&
         m_dragCopy &&
         m_dragCopyIndex == i)) {
      x += tabWidth;
    }

    tab->text = tab->view->getTabText();
    tab->icon = tab->view->getTabIcon();
    tab->color = tab->view->getTabColor();
    tab->x = int(x);
    tab->width = int(x+tabWidth) - int(x);
    x += tabWidth;
    ++i;
  }

  calculateHot();
  invalidate();
}

// Returns true if the user can select other tab.
bool Tabs::canSelectOtherTab() const
{
  return (m_selected && !m_isDragging);
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
  if (!m_selected)
    return;

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
  if (!m_selected)
    return;

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
  m_docked = true;
  initTheme();
}

void Tabs::setDropViewPreview(const gfx::Point& screenPos,
                              TabView* view)
{
  int x0 = (display()->nativeWindow()->pointFromScreen(screenPos).x - bounds().x);
  int newIndex = -1;

  if (!m_list.empty()) {
    newIndex = x0 / m_list[0]->width;
    newIndex = std::clamp(newIndex, 0, (int)m_list.size());
  }
  else
    newIndex = 0;

  bool startAni = (m_dropNewIndex != newIndex ||
                   m_dropNewTab != view);

  m_dropNewIndex = newIndex;
  m_dropNewPosX = x0;
  m_dropNewTab = view;

  if (startAni)
    startReorderTabsAnimation();
  else
    invalidate();
}

void Tabs::removeDropViewPreview()
{
  m_dropNewTab = nullptr;

  startReorderTabsAnimation();
}

bool Tabs::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseEnterMessage:
      calculateHot();
      break;

    case kMouseMoveMessage:
      calculateHot();
      updateDragCopyCursor(msg);

      if (hasCapture() && m_selected) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        const gfx::Point mousePos = mouseMsg->position();
        const gfx::Point screenPos = mouseMsg->screenPosition();
        const gfx::Point delta = mousePos - m_dragMousePos;

        if (!m_isDragging) {
          if (!m_clickedCloseButton && mouseMsg->left()) {
            double dist = std::sqrt(delta.x*delta.x + delta.y*delta.y);
            if (dist > 4.0/ui::guiscale())
              startDrag();
          }
        }
        // We are dragging a tab...
        else {
          // Floating tab (to create a new window)
          if (!bounds().contains(mousePos) &&
              (ABS(delta.y) > 16*guiscale() ||
                mousePos.x < bounds().x-16*guiscale() ||
                mousePos.x > bounds().x2()+16*guiscale())) {
            DropViewPreviewResult result = DropViewPreviewResult::FLOATING;

            if (!m_floatingTab) {
              resetOldPositions();
              m_floatingTab = m_selected;
              startRemoveDragTabAnimation();
            }

            if (m_delegate)
              result = m_delegate->onFloatingTab(
                this, m_selected->view, screenPos);

            if (result != DropViewPreviewResult::DROP_IN_TABS) {
              if (!m_floatingOverlay)
                createFloatingOverlay(m_selected.get());
              m_floatingOverlay->moveOverlay(mousePos - m_floatingOffset);
            }
            else {
              destroyFloatingOverlay();
            }
          }
          else {
            destroyFloatingTab();

            if (m_delegate)
              m_delegate->onDockingTab(this, m_selected->view);
          }

          // Docked tab
          if (!m_floatingTab) {
            m_dragTab->oldX = m_dragTab->x = m_dragTabX + delta.x;
            updateDragTabIndexes(mousePos.x, false);
          }

          invalidate();
        }
      }
      return true;

    case kMouseLeaveMessage:
      if (m_hot) {
        m_hot.reset();
        if (m_delegate)
          m_delegate->onMouseLeaveTab();
        invalidate();
      }
      return true;

    case kMouseDownMessage:
      if (m_hot && !hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        m_dragMousePos = mouseMsg->position();
        m_floatingOffset = mouseMsg->position() -
          (bounds().origin() + getTabBounds(m_hot.get()).origin());

        if (m_hotCloseButton) {
          if (!m_clickedCloseButton) {
            m_clickedCloseButton = true;
            invalidate();
          }
        }
        else if (mouseMsg->left()) {
          selectTabInternal(m_hot);
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
          DropTabResult result = DropTabResult::NOT_HANDLED;

          if (m_delegate) {
            ASSERT(m_selected);
            result = m_delegate->onDropTab(
              this, m_selected->view,
              mouseMsg->screenPosition(), m_dragCopy);
          }

          stopDrag(result);
        }
      }
      return true;

    case kMouseWheelMessage:
      if (!m_isDragging) {
        int dz =
          (static_cast<MouseMessage*>(msg)->wheelDelta().x +
           static_cast<MouseMessage*>(msg)->wheelDelta().y);

        auto it = std::find(m_list.begin(), m_list.end(), m_selected);
        if (it != m_list.end()) {
          int index = (it - m_list.begin());
          int newIndex = index + dz;
          newIndex = std::clamp(newIndex, 0, int(m_list.size())-1);
          if (newIndex != index) {
            selectTabInternal(m_list[newIndex]);
          }
        }
        return true;
      }
      break;

    case kKeyDownMessage:
    case kKeyUpMessage:
      updateDragCopyCursor(msg);
      break;

    case kSetCursorMessage:
      updateMouseCursor();
      return true;

    case kDoubleClickMessage:
      // When we double-click outside tabs (!m_hot), we trigger the
      // double-click in tabs container to show the "New Sprite" dialog.
      if (!m_hot && m_delegate)
        m_delegate->onTabsContainerDoubleClicked(this);
      return true;

  }

  return Widget::onProcessMessage(msg);
}

void Tabs::onInitTheme(ui::InitThemeEvent& ev)
{
  Widget::onInitTheme(ev);
  auto theme = SkinTheme::get(this);

  if (m_docked) {
    m_tabsHeight = theme->dimensions.dockedTabsHeight();
    m_tabsBottomHeight = 0;
    setStyle(theme->styles.workspaceTabs());
  }
  else {
    m_tabsHeight = theme->dimensions.tabsHeight();
    m_tabsBottomHeight = theme->dimensions.tabsBottomHeight();
    setStyle(theme->styles.mainTabs());
  }
}

void Tabs::onPaint(PaintEvent& ev)
{
  auto theme = SkinTheme::get(this);
  Graphics* g = ev.graphics();
  gfx::Rect rect = clientBounds();
  gfx::Rect box(rect.x, rect.y, rect.w,
                m_tabsHeight - m_tabsBottomHeight);

  theme->paintWidget(g, this, style(), rect);

  if (!m_docked)
    drawFiller(g, box);

  // For each tab...
  for (TabPtr& tab : m_list) {
    if (tab == m_floatingTab && !m_dragCopy)
      continue;

    box = getTabBounds(tab.get());

    // The m_dragTab is drawn after all other regular tabs.
    if ((!m_dragTab) ||
        (tab->view != m_dragTab->view) ||
        (m_dragCopy)) {
      int dy = 0;
      if (animation() == ANI_ADDING_TAB && tab == m_addedTab) {
        double t = animationTime();
        dy = int(box.h - box.h * t);
      }

      drawTab(g, box, tab.get(), dy,
              (tab == m_hot),
              (tab == m_selected));
    }

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

  // Tab that is being dragged. It's drawn here so it appears at the
  // front of all other tabs.
  if (m_dragTab && !m_floatingTab) {
    TabPtr tab(m_dragTab);
    box = getTabBounds(tab.get());
    drawTab(g, box, tab.get(), 0, true, true);
  }

  // New tab from other Tab that want to be dropped here.
  if (m_dropNewTab) {
    Tab newTab(m_dropNewTab);

    newTab.width = newTab.oldWidth =
      (!m_list.empty() ? m_list[0]->width:
        theme->dimensions.tabsWidth());

    newTab.x = newTab.oldX =
      m_dropNewPosX - newTab.width/2;

    box = getTabBounds(&newTab);
    drawTab(g, box, &newTab, 0, true, true);
  }
}

void Tabs::onResize(ResizeEvent& ev)
{
  setBoundsQuietly(ev.bounds());
  updateTabs();
}

void Tabs::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(0, m_tabsHeight));
}

void Tabs::selectTabInternal(TabPtr& tab)
{
  if (m_selected != tab) {
    m_selected = tab;
    makeTabVisible(tab.get());
    invalidate();
  }

  if (m_delegate && tab)
    m_delegate->onSelectTab(this, tab->view);
}

void Tabs::drawTab(Graphics* g, const gfx::Rect& _box,
                   Tab* tab, int dy, bool hover, bool selected)
{
  gfx::Rect box = _box;
  if (box.w < ui::guiscale()*8)
    box.w = ui::guiscale()*8;

  auto theme = SkinTheme::get(this);
  int clipTextRightSide;

  gfx::Rect closeBox = getTabCloseButtonBounds(tab, box);
  if (closeBox.isEmpty())
    clipTextRightSide = 4*ui::guiscale();
  else {
    closeBox.y += dy;
    clipTextRightSide = closeBox.w;
  }

  // Tab without text
  PaintWidgetPartInfo info;
  info.styleFlags =
    (selected ? ui::Style::Layer::kFocus: 0) |
    (hover ? ui::Style::Layer::kMouse: 0);
  theme->paintWidgetPart(
    g, theme->styles.tab(),
    gfx::Rect(box.x, box.y+dy, box.w, box.h),
    info);

  gfx::Color tabColor = tab->color;
  gfx::Color textColor = gfx::ColorNone;
  if (tabColor != gfx::ColorNone) {
    textColor = color_utils::blackandwhite_neg(tabColor);
    if (!selected) {
      tabColor = gfx::seta(tabColor, gfx::geta(tabColor)*3/4);
      textColor = gfx::seta(textColor, gfx::geta(textColor)*3/4);
    }
  }

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
          theme->paintWidgetPart(
            g, theme->styles.tabHome(),
            gfx::Rect(
              box.x,
              box.y+dy,
              box.x-dx,
              box.h),
            info);
          dx += theme->dimensions.tabsIconWidth();
        }
        break;
    }

    // Tab with text + clipping the close button
    if (box.w > 8*ui::guiscale()) {
      Style* stylePtr = theme->styles.tabText();
      Style newStyle(nullptr);

      info.text = &tab->text;
      if (tabColor != gfx::ColorNone) {
        // TODO replace these fillRect() with a new theme part (which
        // should be painted with the specific user-defined color)
        g->fillRect(tabColor, gfx::Rect(box.x+dx+2, box.y+dy+3, box.w-dx-2, box.h-3));
        g->fillRect(tabColor, gfx::Rect(box.x+dx+3, box.y+dy+2, box.w-dx-3, 1));

        newStyle = Style(*stylePtr);
        for (auto& layer : newStyle.layers()) {
          if (layer.type() == ui::Style::Layer::Type::kText ||
              layer.type() == ui::Style::Layer::Type::kBackground) {
            layer.setColor(textColor);
          }
        }
        stylePtr = &newStyle;
      }
      theme->paintWidgetPart(
        g, stylePtr,
        gfx::Rect(box.x+dx, box.y+dy, box.w-dx, box.h),
        info);
      info.text = nullptr;
    }
  }

  // Tab bottom part
  if (!m_docked) {
    theme->paintWidgetPart(
      g, theme->styles.tabBottom(),
      gfx::Rect(box.x, box.y2(), box.w, bounds().y2()-box.y2()),
      info);
  }

  // Close button
  if (!closeBox.isEmpty()) {
    ui::Style* style = theme->styles.tabCloseIcon();

    if (m_delegate) {
      if (tab->view)
        tab->modified = m_delegate->isTabModified(this, tab->view);

      if (tab->modified &&
          (!hover || !m_hotCloseButton)) {
        style = theme->styles.tabModifiedIcon();
      }
    }

    if (tabColor != gfx::ColorNone) {
      g->fillRect(tabColor, gfx::Rect(closeBox.x, closeBox.y+3, closeBox.w-3, closeBox.h-3));
      g->fillRect(tabColor, gfx::Rect(closeBox.x, closeBox.y+2, closeBox.w-4, 1));
    }

    info.styleFlags = 0;
    if (selected)
      info.styleFlags |= ui::Style::Layer::kFocus;
    if (hover && m_hotCloseButton) {
      info.styleFlags |= ui::Style::Layer::kMouse;
      if (m_clickedCloseButton)
        info.styleFlags |= ui::Style::Layer::kSelected;
    }

    theme->paintWidgetPart(g, style, closeBox, info);
  }
}

void Tabs::drawFiller(ui::Graphics* g, const gfx::Rect& box)
{
  auto theme = SkinTheme::get(this);
  gfx::Rect rect = clientBounds();

  theme->paintWidgetPart(
    g, theme->styles.tabFiller(),
    gfx::Rect(box.x, box.y, rect.x2()-box.x, box.h),
    PaintWidgetPartInfo());

  theme->paintWidgetPart(
    g, theme->styles.tabBottom(),
    gfx::Rect(box.x, box.y2(), rect.x2()-box.x, rect.y2()-box.y2()),
    PaintWidgetPartInfo());
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

  gfx::Rect rect = bounds();
  gfx::Rect box(rect.x+m_border*guiscale(), rect.y, 0, rect.h-1);
  gfx::Point mousePos = mousePosInDisplay();
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
  auto theme = SkinTheme::get(this);
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

    ASSERT(tab->x != (int)0xfefefefe);
    ASSERT(tab->width != (int)0xfefefefe);
    ASSERT(tab->oldX != (int)0xfefefefe);
    ASSERT(tab->oldWidth != (int)0xfefefefe);

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
  m_addedTab.reset();

  if (m_list.empty()) {
    Widget* root = window();
    if (root)
      root->layout();
  }
}

void Tabs::startDrag()
{
  ASSERT(m_selected);

  updateTabs();

  m_isDragging = true;
  m_dragTab.reset(new Tab(m_selected->view));
  m_dragTab->oldX = m_dragTab->x = m_dragTabX = m_selected->x;
  m_dragTab->oldWidth = m_dragTab->width = m_selected->width;

  m_dragTabIndex =
  m_dragCopyIndex = std::find(m_list.begin(), m_list.end(), m_selected) - m_list.begin();

  EditorView::SetScrollUpdateMethod(EditorView::KeepCenter);
}

void Tabs::stopDrag(DropTabResult result)
{
  m_isDragging = false;
  ASSERT(m_dragTab);

  switch (result) {

    case DropTabResult::NOT_HANDLED:
    case DropTabResult::DONT_REMOVE: {
      destroyFloatingTab();

      bool localCopy = false;
      if (result == DropTabResult::NOT_HANDLED &&
          m_dragTab && m_dragCopy && m_delegate) {
        ASSERT(m_dragCopyIndex >= 0);

        m_delegate->onCloneTab(this, m_dragTab->view, m_dragCopyIndex);

        // To animate the new tab created by onCloneTab() from the
        // m_dragTab position.
        m_list[m_dragCopyIndex]->oldX = m_dragTab->x;
        m_list[m_dragCopyIndex]->oldWidth = m_dragTab->width;
        localCopy = true;
      }

      m_dragCopyIndex = -1;

      startReorderTabsAnimation();

      if (m_selected && m_dragTab && !localCopy) {
        if (result == DropTabResult::NOT_HANDLED) {
          // To animate m_selected tab from the m_dragTab position
          // when we drop the tab in the same Tabs (with no copy)
          m_selected->oldX = m_dragTab->x;
          m_selected->oldWidth = m_dragTab->width;
        }
        else {
          ASSERT(result == DropTabResult::DONT_REMOVE);

          // In this case the tab was copied to other Tabs, so we
          // avoid any kind of animation for the m_selected (it stays
          // were it's).
          m_selected->oldX = m_selected->x;
          m_selected->oldWidth = m_selected->width;
        }
      }
      break;
    }

    case DropTabResult::REMOVE:
      m_floatingTab.reset();
      m_removedTab.reset();
      m_dragCopyIndex = -1;
      destroyFloatingTab();

      ASSERT(m_dragTab.get());
      if (m_dragTab)
        removeTab(m_dragTab->view, false);
      break;

  }

  m_dragTab.reset();

  EditorView::SetScrollUpdateMethod(EditorView::KeepOrigin);
}

gfx::Rect Tabs::getTabBounds(Tab* tab)
{
  gfx::Rect rect = clientBounds();
  gfx::Rect box(rect.x, rect.y, rect.w, m_tabsHeight - m_tabsBottomHeight);
  int startX = m_border*guiscale();
  double t = animationTime();

  if (animation() == ANI_NONE) {
    box.x = startX + tab->x;
    box.w = tab->width;
  }
  else {
    ASSERT(tab->x != (int)0xfefefefe);
    ASSERT(tab->width != (int)0xfefefefe);
    ASSERT(tab->oldX != (int)0xfefefefe);
    ASSERT(tab->oldWidth != (int)0xfefefefe);

    box.x = startX + int(inbetween(tab->oldX, tab->x, t));
    box.w = int(inbetween(tab->oldWidth, tab->width, t));
  }

  return box;
}

void Tabs::startReorderTabsAnimation()
{
  resetOldPositions(animationTime());
  updateTabs();
  startAnimation(ANI_REORDER_TABS, ANI_REORDER_TABS_TICKS);
}

void Tabs::startRemoveDragTabAnimation()
{
  m_removedTab.reset();
  updateTabs();
  startAnimation(ANI_REMOVING_TAB, ANI_REMOVING_TAB_TICKS);
}

void Tabs::createFloatingOverlay(Tab* tab)
{
  ASSERT(!m_floatingOverlay);

  ui::Display* display = this->display();
  os::SurfaceRef surface = os::instance()->makeRgbaSurface(
    tab->width, m_tabsHeight);

  // Fill the surface with pink color
  {
    os::SurfaceLock lock(surface.get());
    os::Paint paint;
    paint.color(gfx::rgba(0, 0, 0, 0));
    paint.style(os::Paint::Fill);
    surface->drawRect(gfx::Rect(0, 0, surface->width(), surface->height()), paint);
  }
  {
    Graphics g(display, surface, 0, 0);
    g.setFont(AddRef(font()));
    drawTab(&g, g.getClipBounds(), tab, 0, true, true);
  }

  surface->setImmutable();

  m_floatingOverlay = base::make_ref<ui::Overlay>(
    display, surface, gfx::Point(),
    (ui::Overlay::ZOrder)(Overlay::MouseZOrder-1));
  OverlayManager::instance()->addOverlay(m_floatingOverlay);
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

    m_addedTab = tab;
  }
}

void Tabs::destroyFloatingOverlay()
{
  if (m_floatingOverlay) {
    OverlayManager::instance()->removeOverlay(m_floatingOverlay);
    m_floatingOverlay.reset();
  }
}

void Tabs::updateMouseCursor()
{
  if (m_dragCopy)
    ui::set_mouse_cursor(kArrowPlusCursor);
  else
    ui::set_mouse_cursor(kArrowCursor);
}

void Tabs::updateDragTabIndexes(int mouseX, bool startAni)
{
  if (m_dragTab) {
    int i = (mouseX - m_border*guiscale() - bounds().x) / m_dragTab->width;

    if (m_dragCopy) {
      i = std::clamp(i, 0, int(m_list.size()));
      if (i != m_dragCopyIndex) {
        m_dragCopyIndex = i;
        startAni = true;
      }
    }
    else if (hasMouse()) {
      i = std::clamp(i, 0, int(m_list.size())-1);
      if (i != m_dragTabIndex) {
        m_list.erase(m_list.begin()+m_dragTabIndex);
        m_list.insert(m_list.begin()+i, m_selected);
        m_dragTabIndex = i;
        startAni = true;
      }
    }
  }

  if (startAni)
    startReorderTabsAnimation();
}

void Tabs::updateDragCopyCursor(ui::Message* msg)
{
  TabPtr tab = (m_isDragging ? m_dragTab: m_hot);

  bool oldDragCopy = m_dragCopy;
  m_dragCopy = ((
#if !defined __APPLE__
                 msg->ctrlPressed() ||
#endif
                 msg->altPressed()) &&
                (tab && m_delegate && m_delegate->canCloneTab(this, tab->view)));

  if (oldDragCopy != m_dragCopy) {
    updateDragTabIndexes(mousePosInDisplay().x, true);
    updateMouseCursor();
  }
}

} // namespace app
