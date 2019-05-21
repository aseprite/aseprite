// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_WORKSPACE_VIEW_H_INCLUDED
#define APP_UI_WORKSPACE_VIEW_H_INCLUDED
#pragma once

namespace ui {
  class Widget;
}

namespace app {
  class InputChainElement;
  class Workspace;

  class WorkspaceView {
  public:
    virtual ~WorkspaceView() { }

    virtual ui::Widget* getContentWidget() = 0;

    virtual bool canCloneWorkspaceView() { return false; }
    virtual WorkspaceView* cloneWorkspaceView() { return nullptr; }

    // Called after the view is added in the correct position inside
    // the workspace. It can be used to copy/clone scroll position
    // from the original view.
    virtual void onClonedFrom(WorkspaceView* from) {
      // Do nothing
    }

    virtual void onWorkspaceViewSelected() = 0;

    // Returns true if the view was closed successfully or false if
    // the user cancels the operation.
    virtual bool onCloseView(Workspace* workspace, bool quitting) = 0;

    virtual void onAfterRemoveView(Workspace* workspace) { }

    virtual void onTabPopup(Workspace* workspace) = 0;

    virtual InputChainElement* onGetInputChainElement() { return nullptr; }
  };

} // namespace app

#endif
