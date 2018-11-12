// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TABS_H_INCLUDED
#define APP_UI_TABS_H_INCLUDED
#pragma once

#include "app/ui/animated_widget.h"
#include "base/shared_ptr.h"
#include "ui/mouse_buttons.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <vector>

namespace ui {
  class Graphics;
  class Overlay;
}

namespace app {
  class Tabs;

  enum class TabIcon {
    NONE,
    HOME,
  };

  // Required interface to be implemented by each new tab that is added
  // in the Tabs widget.
  class TabView {
  public:
    virtual ~TabView() { }

    // Returns the text to be shown in the tab.
    virtual std::string getTabText() = 0;

    // Returns the icon to be shown in the tab
    virtual TabIcon getTabIcon() = 0;
  };

  enum class DropTabResult {
    // The operation should be handled inside the Tabs widget (if the
    // tab was dropped in the same Tabs, it will be moved or copied
    // depending on what the user wants).
    NOT_HANDLED,

    // The tab was docked in other place, so it must be removed from
    // the specific Tabs widget.
    REMOVE,

    // The operation was already handled, but the tab must not be
    // removed from the Tabs (e.g. because it was cloned).
    DONT_REMOVE,
  };

  enum class DropViewPreviewResult {
    DROP_IN_PANEL,
    DROP_IN_TABS,
    FLOATING,
  };

  // Interface used to control notifications from the Tabs widget.
  class TabsDelegate {
  public:

    virtual ~TabsDelegate() { }

    // Returns true if the tab represent a modified document.
    virtual bool isTabModified(Tabs* tabs, TabView* tabView) = 0;

    // Returns true if the tab can be cloned.
    virtual bool canCloneTab(Tabs* tabs, TabView* tabView) = 0;

    // Called when the user selected the tab with the left mouse button.
    virtual void onSelectTab(Tabs* tabs, TabView* tabView) = 0;

    // When the tab close button is pressed (or middle mouse button is used to close it).
    virtual void onCloseTab(Tabs* tabs, TabView* tabView) = 0;

    // When the tab is cloned (this only happens when the user
    // drag-and-drop the tab into the same Tabs with the Ctrl key).
    virtual void onCloneTab(Tabs* tabs, TabView* tabView, int pos) = 0;

    // When the right-click is pressed in the tab.
    virtual void onContextMenuTab(Tabs* tabs, TabView* tabView) = 0;

    // When the tab bar background is double-clicked.
    virtual void onTabsContainerDoubleClicked(Tabs* tabs) = 0;

    // Called when the mouse is over a tab (the data can be null if the
    // mouse just leave all tabs)
    virtual void onMouseOverTab(Tabs* tabs, TabView* tabView) = 0;

    // Called when the mouse is leaving a tab
    virtual void onMouseLeaveTab() = 0;

    // Called when the user is dragging a tab outside the Tabs
    // bar.
    virtual DropViewPreviewResult onFloatingTab(Tabs* tabs, TabView* tabView, const gfx::Point& pos) = 0;

    // Called when the user is dragging a tab inside the Tabs bar.
    virtual void onDockingTab(Tabs* tabs, TabView* tabView) = 0;

    virtual DropTabResult onDropTab(Tabs* tabs, TabView* tabView, const gfx::Point& pos, bool clone) = 0;
  };

  // Tabs control. Used to show opened documents.
  class Tabs : public ui::Widget
             , public AnimatedWidget {
    struct Tab {
      TabView* view;
      std::string text;
      TabIcon icon;
      int x, width;
      int oldX, oldWidth;
      bool modified;

      Tab(TabView* view) : view(view) {
        ASSERT(view);
        text = view->getTabText();
        icon = view->getTabIcon();

        x = width = oldX = oldWidth =
#if _DEBUG
          0xfefefefe;
#else
          0;
#endif

        modified = false;
      }
    };

    typedef base::SharedPtr<Tab> TabPtr;

    typedef std::vector<TabPtr> TabsList;
    typedef TabsList::iterator TabsListIterator;

    enum Ani : int {
      ANI_NONE,
      ANI_ADDING_TAB,
      ANI_REMOVING_TAB,
      ANI_REORDER_TABS
    };

  public:
    static ui::WidgetType Type();

    Tabs(TabsDelegate* delegate);
    ~Tabs();

    TabsDelegate* getDelegate() { return m_delegate; }

    void addTab(TabView* tabView, bool from_drop, int pos = -1);
    void removeTab(TabView* tabView, bool with_animation);
    void updateTabs();

    // Returns true if the user can select other tab.
    bool canSelectOtherTab() const;

    void selectTab(TabView* tabView);
    void selectNextTab();
    void selectPreviousTab();
    TabView* getSelectedTab();

    void setDockedStyle();

    // Drop TabViews into this Tabs widget
    void setDropViewPreview(const gfx::Point& pos, TabView* view);
    void removeDropViewPreview();
    int getDropTabIndex() const { return m_dropNewIndex; }

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onInitTheme(ui::InitThemeEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onAnimationFrame() override;
    void onAnimationStop(int animation) override;

  private:
    void resetOldPositions();
    void resetOldPositions(double t);

    void selectTabInternal(TabPtr& tab);
    void drawTab(ui::Graphics* g, const gfx::Rect& box, Tab* tab, int dy, bool hover, bool selected);
    void drawFiller(ui::Graphics* g, const gfx::Rect& box);
    TabsListIterator getTabIteratorByView(TabView* tabView);
    TabPtr getTabByView(TabView* tabView);
    int getMaxScrollX();
    void makeTabVisible(Tab* tab);
    void calculateHot();
    gfx::Rect getTabCloseButtonBounds(Tab* tab, const gfx::Rect& box);
    void startDrag();
    void stopDrag(DropTabResult result);
    gfx::Rect getTabBounds(Tab* tab);
    void startReorderTabsAnimation();
    void startRemoveDragTabAnimation();
    void createFloatingOverlay(Tab* tab);
    void destroyFloatingTab();
    void destroyFloatingOverlay();
    void updateMouseCursor();
    void updateDragTabIndexes(int mouseX, bool force_animation);
    void updateDragCopyCursor(ui::Message* msg);

    // Specific variables about the style
    int m_border;           // Pixels used from the left side to draw the first tab
    bool m_docked;          // True if tabs are inside the workspace (not the main tabs panel)
    int m_tabsHeight;       // Number of pixels in Y-axis for each Tab
    int m_tabsBottomHeight; // Number of pixels in the bottom part of Tabs widget

    // List of tabs (pointers to Tab instances).
    TabsList m_list;

    // Which tab has the mouse over.
    TabPtr m_hot;

    // True if the mouse is above the close button of m_hot tab.
    bool m_hotCloseButton;

    // True if the user clicked over the close button of m_hot.
    bool m_clickedCloseButton;

    // Current active tab. When this tab changes, the
    // TabsDelegate::onSelectTab() is called.
    TabPtr m_selected;

    // Delegate of notifications
    TabsDelegate* m_delegate;

    // Variables for animation purposes
    TabPtr m_removedTab;

    ////////////////////////////////////////
    // Drag-and-drop

    // True when the user is dragging a tab (the mouse must be
    // captured when this flag is true). The dragging process doesn't
    // start immediately, when the user clicks a tab the m_selected
    // tab is changed, and then there is a threshold after we start
    // dragging the tab.
    bool m_isDragging;

    // True when the user is dragging the tab as a copy inside this
    // same Tabs (e.g. using Ctrl key) This can be true only if
    // TabsDelegate::canCloneTab() returns true for the tab being
    // dragged.
    bool m_dragCopy;

    // A copy of m_selected used only in the dragging process. This
    // tab is not pointing to the same tab as m_selected. It's a copy
    // because if m_dragCopy is true, we've to paint m_selected and
    // m_dragTab separately (as two different tabs).
    TabPtr m_dragTab;

    // Initial X position where m_dragTab/m_selected was when we
    // started (mouse down) the drag process.
    int m_dragTabX;

    // Initial mouse position when we start the dragging process.
    gfx::Point m_dragMousePos;

    // New position where m_selected is being dragged. It's used to
    // change the position of m_selected inside the m_list vector in
    // real-time while the mouse is moved.
    int m_dragTabIndex;

    // New position where a copyt of m_dragTab will be dropped. It
    // only makes sense when m_dragCopy is true (the user is pressing
    // Ctrl key and want to create a copy of the tab).
    int m_dragCopyIndex;

    // Null if we are dragging the m_dragTab inside this Tabs widget,
    // or non-null (equal to m_selected indeed), if we're moving the
    // tab outside the Tabs widget (e.g. to dock the tabs in other
    // location).
    TabPtr m_floatingTab;

    // Overlay used to show the floating tab outside the Tabs widget
    // (this overlay floats next to the mouse cursor).  It's destroyed
    // and recreated every time the tab is put inside or outside the
    // Tabs widget.
    std::unique_ptr<ui::Overlay> m_floatingOverlay;

    // Relative mouse position inside the m_dragTab (used to adjust
    // the m_floatingOverlay precisely).
    gfx::Point m_floatingOffset;

    ////////////////////////////////////////
    // Drop new tabs

    // Non-null when a foreign tab (a "TabView" from other "Tabs"
    // widget), will be dropped in this specific Tabs widget. It's
    // used only for feedback/UI purposes when the
    // setDropViewPreview() is called.
    TabView* m_dropNewTab;
    int m_dropNewIndex;
    int m_dropNewPosX;
  };

} // namespace app

#endif
