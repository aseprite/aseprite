// Aseprite
// Copyright (C)      2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/ui/doc_view.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"

namespace app {

using namespace ui;

class CloseFileCommand : public Command {
public:
  CloseFileCommand() : Command(CommandId::CloseFile()) {}

protected:
  bool onEnabled(Context* context) override
  {
    Workspace* workspace = App::instance()->workspace();
    if (!workspace) // Workspace (main window) can be null if we are in --batch mode
      return false;
    WorkspaceView* view = workspace->activeView();
    return (view != nullptr);
  }

  void onExecute(Context* context) override
  {
    Workspace* workspace = App::instance()->workspace();
    WorkspaceView* view = workspace->activeView();
    if (view)
      workspace->closeView(view, false);
  }
};

class CloseAllFilesCommand : public Command {
public:
  CloseAllFilesCommand() : Command(CommandId::CloseAllFiles()) { m_quitting = false; }

protected:
  bool onEnabled(Context* context) override
  {
    // Null if we are in --batch mode
    return App::instance()->workspace() != nullptr;
  }

  void onLoadParams(const Params& params) override { m_quitting = params.get_as<bool>("quitting"); }

  void onExecute(Context* context) override
  {
    Workspace* workspace = App::instance()->workspace();
    if (!workspace) // Workspace (main window) can be null if we are in --batch mode
      return;

    // Collect all document views
    DocViews docViews;
    for (auto view : *workspace) {
      DocView* docView = dynamic_cast<DocView*>(view);
      if (docView)
        docViews.push_back(docView);
    }

    for (auto docView : docViews) {
      if (!workspace->closeView(docView, m_quitting))
        break;
    }
  }

private:
  bool m_quitting;
};

Command* CommandFactory::createCloseFileCommand()
{
  return new CloseFileCommand;
}

Command* CommandFactory::createCloseAllFilesCommand()
{
  return new CloseAllFilesCommand;
}

} // namespace app
