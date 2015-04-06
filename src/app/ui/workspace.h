// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_WORKSPACE_H_INCLUDED
#define APP_UI_WORKSPACE_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "app/ui/workspace_panel.h"
#include "base/signal.h"
#include "ui/widget.h"

namespace app {
  class WorkspaceTabs;

  class Workspace : public ui::Widget {
  public:
    typedef WorkspaceViews::iterator iterator;

    static ui::WidgetType Type();

    Workspace();
    ~Workspace();

    void setTabsBar(WorkspaceTabs* tabs);

    iterator begin() { return m_views.begin(); }
    iterator end() { return m_views.end(); }

    void addView(WorkspaceView* view, int pos = -1);
    void removeView(WorkspaceView* view);

    // Closes the given view. Returns false if the user cancels the
    // operation.
    bool closeView(WorkspaceView* view);

    WorkspaceView* activeView();
    void setActiveView(WorkspaceView* view);
    void setMainPanelAsActive();
    void selectNextTab();
    void selectPreviousTab();
    void duplicateActiveView();

    // Set the preview of what could happen if we drop the given
    // "view" at the "pos"?
    DropViewPreviewResult setDropViewPreview(const gfx::Point& pos,
      WorkspaceView* view, WorkspaceTabs* tabs);
    void removeDropViewPreview();

    // Returns true if the view was docked inside the workspace.
    bool dropViewAt(const gfx::Point& pos, WorkspaceView* view);

    Signal0<void> ActiveViewChanged;

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;

  private:
    void addViewToPanel(WorkspacePanel* panel, WorkspaceView* view, bool from_drop, int pos);
    WorkspacePanel* getViewPanel(WorkspaceView* view);
    WorkspacePanel* getPanelAt(const gfx::Point& pos);
    WorkspaceTabs* getTabsAt(const gfx::Point& pos);

    WorkspacePanel m_mainPanel;
    WorkspaceTabs* m_tabs;
    WorkspaceViews m_views;
    WorkspacePanel* m_activePanel;
    WorkspacePanel* m_dropPreviewPanel;
    WorkspaceTabs* m_dropPreviewTabs;
  };

} // namespace app

#endif
