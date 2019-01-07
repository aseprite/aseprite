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
#include "app/context.h"
#include "app/doc.h"
#include "app/job.h"
#include "app/ui/main_window.h"
#include "ui/alert.h"

namespace app {

class ExitCommand : public Command {
public:
  ExitCommand();

protected:
  void onExecute(Context* context) override;
};

ExitCommand::ExitCommand()
  : Command(CommandId::Exit(), CmdUIOnlyFlag)
{
}

void ExitCommand::onExecute(Context* ctx)
{
  // Ignore ExitCommand when we are saving documents or doing a
  // background task
  if (Job::runningJobs() > 0)
    return;

  if (ctx->hasModifiedDocuments()) {
    Command* closeAll = Commands::instance()->byId(CommandId::CloseAllFiles());
    Params params;
    params.set("quitting", "1");
    ctx->executeCommand(closeAll, params);

    // The user didn't save all documents (canceled the exit)
    if (ctx->hasModifiedDocuments())
      return;
  }

  // Close the window
  App::instance()->mainWindow()->closeWindow(NULL);
}

Command* CommandFactory::createExitCommand()
{
  return new ExitCommand;
}

} // namespace app
