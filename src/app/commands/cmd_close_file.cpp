// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/document_access.h"
#include "app/modules/editors.h"
#include "app/ui/document_view.h"
#include "app/ui/status_bar.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "doc/sprite.h"
#include "ui/ui.h"

#include <memory>

namespace app {

using namespace ui;

class CloseFileCommand : public Command {
public:
  CloseFileCommand()
    : Command(CommandId::CloseFile(), CmdUIOnlyFlag) {
  }

  Command* clone() const override { return new CloseFileCommand(*this); }

protected:

  bool onEnabled(Context* context) override {
    Workspace* workspace = App::instance()->workspace();
    WorkspaceView* view = workspace->activeView();
    return (view != nullptr);
  }

  void onExecute(Context* context) override {
    Workspace* workspace = App::instance()->workspace();
    WorkspaceView* view = workspace->activeView();
    if (view)
      workspace->closeView(view, false);
  }
};

class CloseAllFilesCommand : public Command {
public:
  CloseAllFilesCommand()
    : Command(CommandId::CloseAllFiles(), CmdRecordableFlag) {
    m_quitting = false;
  }

  Command* clone() const override { return new CloseAllFilesCommand(*this); }

protected:

  void onLoadParams(const Params& params) override {
    m_quitting = params.get_as<bool>("quitting");
  }

  void onExecute(Context* context) override {
    Workspace* workspace = App::instance()->workspace();

    // Collect all document views
    DocumentViews docViews;
    for (auto view : *workspace) {
      DocumentView* docView = dynamic_cast<DocumentView*>(view);
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
