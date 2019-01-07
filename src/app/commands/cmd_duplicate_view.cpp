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
#include "app/ui/workspace.h"

#include <cstdio>

namespace app {

// using namespace ui;

class DuplicateViewCommand : public Command {
public:
  DuplicateViewCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DuplicateViewCommand::DuplicateViewCommand()
  : Command(CommandId::DuplicateView(), CmdUIOnlyFlag)
{
}

bool DuplicateViewCommand::onEnabled(Context* context)
{
  Workspace* workspace = App::instance()->workspace();
  WorkspaceView* view = workspace->activeView();
  return (view != nullptr);
}

void DuplicateViewCommand::onExecute(Context* context)
{
  App::instance()->workspace()->duplicateActiveView();
}

Command* CommandFactory::createDuplicateViewCommand()
{
  return new DuplicateViewCommand;
}

} // namespace app
