// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_WORKSPACE_H_INCLUDED
#define APP_UI_WORKSPACE_H_INCLUDED
#pragma once

#include "app/ui/input_chain_element.h"
#include "app/ui/tabs.h"
#include "app/ui/workspace_panel.h"
#include "obs/signal.h"
#include "ui/widget.h"

namespace app {
  class WorkspaceTabs;

  class Workspace : public ui::Widget
                  , public app::InputChainElement {
  public:
    typedef WorkspaceViews::iterator iterator;

    static ui::WidgetType Type();

    Workspace();
    ~Workspace();

    void setTabsBar(WorkspaceTabs* tabs);

    iterator begin() { return m_views.begin(); }
    iterator end() { return m_views.end(); }

    void addView(WorkspaceView* view, int pos = -1);
    void addViewToPanel(WorkspacePanel* panel, WorkspaceView* view, bool from_drop, int pos);
    void removeView(WorkspaceView* view);

    // Closes the given view. Returns false if the user cancels the
    // operation.
    bool closeView(WorkspaceView* view, bool quitting);

    WorkspaceView* activeView();
    void setActiveView(WorkspaceView* view);
    void setMainPanelAsActive();
    bool canSelectOtherTab() const;
    void selectNextTab();
    void selectPreviousTab();
    void duplicateActiveView();

    void updateTabs();

    // Set the preview of what could happen if we drop the given
    // "view" at the "pos"?
    DropViewPreviewResult setDropViewPreview(const gfx::Point& pos,
      WorkspaceView* view, WorkspaceTabs* tabs);
    void removeDropViewPreview();

    // Returns true if the view was docked inside the workspace.
    DropViewAtResult dropViewAt(const gfx::Point& pos, WorkspaceView* view, bool clone);

    // InputChainElement impl
    void onNewInputPriority(InputChainElement* element,
                            const ui::Message* msg) override;
    bool onCanCut(Context* ctx) override;
    bool onCanCopy(Context* ctx) override;
    bool onCanPaste(Context* ctx) override;
    bool onCanClear(Context* ctx) override;
    bool onCut(Context* ctx) override;
    bool onCopy(Context* ctx) override;
    bool onPaste(Context* ctx) override;
    bool onClear(Context* ctx) override;
    void onCancel(Context* ctx) override;

    WorkspacePanel* mainPanel() { return &m_mainPanel; }

    obs::signal<void()> ActiveViewChanged;

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;

  private:
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
