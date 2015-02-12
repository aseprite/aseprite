// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_WORKSPACE_PART_H_INCLUDED
#define APP_UI_WORKSPACE_PART_H_INCLUDED
#pragma once

#include "app/ui/workspace_views.h"
#include "ui/box.h"
#include <vector>

namespace app {

  class WorkspacePart : public ui::Box {
  public:
    WorkspacePart();
    ~WorkspacePart();

    size_t getViewCount() { return m_views.size(); }

    void addView(WorkspaceView* view);
    void removeView(WorkspaceView* view);

    WorkspaceView* activeView() { return m_activeView; }
    void setActiveView(WorkspaceView* view);

    bool hasView(WorkspaceView* view);

  private:
    WorkspaceView* m_activeView;
    WorkspaceViews m_views;
  };

} // namespace app

#endif
