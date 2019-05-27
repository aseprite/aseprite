// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/recent_files.h"

namespace app {

class ClearRecentFilesCommand : public Command {
public:
  ClearRecentFilesCommand();

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

ClearRecentFilesCommand::ClearRecentFilesCommand()
  : Command(CommandId::ClearRecentFiles(), CmdUIOnlyFlag)
{
}

bool ClearRecentFilesCommand::onEnabled(Context* ctx)
{
  auto recent = App::instance()->recentFiles();
  return (recent &&
          (!recent->recentFiles().empty() ||
           !recent->recentFolders().empty()));
}

void ClearRecentFilesCommand::onExecute(Context* ctx)
{
  App::instance()->recentFiles()->clear();
}

Command* CommandFactory::createClearRecentFilesCommand()
{
  return new ClearRecentFilesCommand;
}

} // namespace app
