// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_TABS_H_INCLUDED
#define APP_UI_TABS_H_INCLUDED
#pragma once

#include "app/ui/animated_widget.h"
#include "base/shared_ptr.h"
#include "base/unique_ptr.h"
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
    IGNORE,
    DOCKED_IN_OTHER_PLACE,
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
    virtual bool onIsModified(Tabs* tabs, TabView* tabView) = 0;

    // Called when the user selected the tab with the left mouse button.
    virtual void onSelectTab(Tabs* tabs, TabView* tabView) = 0;

    // When the tab close button is pressed (or middle mouse button is used to close it).
    virtual void onCloseTab(Tabs* tabs, TabView* tabView) = 0;

    // When the right-click is pressed in the tab.
    virtual void onContextMenuTab(Tabs* tabs, TabView* tabView) = 0;

    // Called when the mouse is over a tab (the data can be null if the
    // mouse just leave all tabs)
    virtual void onMouseOverTab(Tabs* tabs, TabView* tabView) = 0;

    // Called when the user is dragging a tab outside the Tabs
    // bar.
    virtual DropViewPreviewResult onFloatingTab(Tabs* tabs, TabView* tabView, const gfx::Point& pos) = 0;

    // Called when the user is dragging a tab inside the Tabs bar.
    virtual void onDockingTab(Tabs* tabs, TabView* tabView) = 0;

    virtual DropTabResult onDropTab(Tabs* tabs, TabView* tabView, const gfx::Point& pos) = 0;
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

    void addTab(TabView* tabView, int pos = -1);
    void removeTab(TabView* tabView, bool with_animation);
    void updateTabs();

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
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;
    void onPreferredSize(ui::PreferredSizeEvent& ev) override;
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
    void createFloatingTab(TabPtr& tab);
    void destroyFloatingTab();
    void destroyFloatingOverlay();

    int m_border;
    TabsList m_list;
    TabPtr m_hot;
    bool m_hotCloseButton;
    bool m_clickedCloseButton;
    TabPtr m_selected;

    // Style
    bool m_docked;
    int m_tabsHeight;
    int m_tabsEmptyHeight;

    // Delegate of notifications
    TabsDelegate* m_delegate;

    // Variables for animation purposes
    TabPtr m_removedTab;

    // Drag-and-drop
    bool m_isDragging;
    int m_dragTabX;
    gfx::Point m_dragMousePos;
    gfx::Point m_dragOffset;
    int m_dragTabIndex;
    TabPtr m_floatingTab;
    base::UniquePtr<ui::Overlay> m_floatingOverlay;

    // Drop new tabs
    TabView* m_dropNewTab;
    int m_dropNewIndex;
    int m_dropNewPosX;
  };

} // namespace app

#endif
