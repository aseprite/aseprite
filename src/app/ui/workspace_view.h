// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_WORKSPACE_VIEW_H_INCLUDED
#define APP_UI_WORKSPACE_VIEW_H_INCLUDED
#pragma once

namespace ui {
  class Widget;
}

namespace app {
  class Workspace;

  class WorkspaceView {
  public:
    virtual ~WorkspaceView() { }

    virtual ui::Widget* getContentWidget() = 0;
    virtual WorkspaceView* cloneWorkspaceView() = 0;
    virtual void onWorkspaceViewSelected() = 0;

    // Called after the view is added in the correct position inside
    // the workspace. It can be used to copy/clone scroll position
    // from the original view.
    virtual void onClonedFrom(WorkspaceView* from) = 0;

    virtual void onCloseView(Workspace* workspace) = 0;
  };

} // namespace app

#endif
