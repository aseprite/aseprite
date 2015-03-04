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

  class Workspace : public ui::Widget {
  public:
    typedef WorkspaceViews::iterator iterator;

    Workspace();
    ~Workspace();

    iterator begin() { return m_views.begin(); }
    iterator end() { return m_views.end(); }

    void addView(WorkspaceView* view);
    void removeView(WorkspaceView* view);

    // Closes the given view. Returns false if the user cancels the
    // operation.
    bool closeView(WorkspaceView* view);

    WorkspaceView* activeView();
    void setActiveView(WorkspaceView* view);

    Signal0<void> ActiveViewChanged;

  protected:
    void onPaint(ui::PaintEvent& ev) override;

  private:
    WorkspaceViews m_views;
    WorkspaceView* m_activeView;
  };

} // namespace app

#endif
