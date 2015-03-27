// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_WORKSPACE_H_INCLUDED
#define APP_UI_WORKSPACE_H_INCLUDED
#pragma once

#include "app/ui/workspace_views.h"
#include "base/signal.h"
#include "ui/widget.h"

#include <map>
#include <vector>

namespace app {
  class Tabs;

  class Workspace : public ui::Widget {
  public:
    typedef WorkspaceViews::iterator iterator;

    Workspace();
    ~Workspace();

    void setTabsBar(Tabs* tabsBar);
    iterator begin() { return m_views.begin(); }
    iterator end() { return m_views.end(); }

    void addView(WorkspaceView* view, int pos = -1);
    void removeView(WorkspaceView* view);

    // Closes the given view. Returns false if the user cancels the
    // operation.
    bool closeView(WorkspaceView* view);

    WorkspaceView* activeView();
    void setActiveView(WorkspaceView* view);

    // Drop views into workspace
    void setDropViewPreview(const gfx::Point& pos);
    void removeDropViewPreview(const gfx::Point& pos);
    void dropViewAt(const gfx::Point& pos, WorkspaceView* view);

    Signal0<void> ActiveViewChanged;

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    void onResize(ui::ResizeEvent& ev) override;

  private:
    int calculateDropArea(const gfx::Point& pos) const;
    int getDropThreshold() const;

    Tabs* m_tabsBar;
    WorkspaceViews m_views;
    WorkspaceView* m_activeView;
    int m_dropArea;
  };

} // namespace app

#endif
