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
#include "ui/box.h"
#include <vector>

namespace app {
  class WorkspacePart;

  class Workspace : public ui::Box {
  public:
    typedef WorkspaceViews::iterator iterator;

    Workspace();
    ~Workspace();

    iterator begin() { return m_views.begin(); }
    iterator end() { return m_views.end(); }

    void addView(WorkspaceView* view);
    void removeView(WorkspaceView* view);

    WorkspaceView* activeView();
    void setActiveView(WorkspaceView* view);

    void splitView(WorkspaceView* view, int orientation);
    void makeUnique(WorkspaceView* view);

    Signal0<void> ActiveViewChanged;

  private:
    typedef std::vector<WorkspacePart*> WorkspaceParts;

    WorkspacePart* destroyPart(WorkspacePart* part);
    WorkspacePart* getPartByView(WorkspaceView* view);
    void enumAllParts(WorkspaceParts& parts);

    // All views of all parts.
    WorkspaceViews m_views;
    WorkspacePart* m_activePart;
  };

} // namespace app

#endif
