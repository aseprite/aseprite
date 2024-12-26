// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ui/main_window.h"

namespace app {

using namespace ui;

class DeveloperConsoleCommand : public Command {
public:
  DeveloperConsoleCommand();
  ~DeveloperConsoleCommand();

protected:
  void onExecute(Context* context);
};

DeveloperConsoleCommand::DeveloperConsoleCommand()
  : Command(CommandId::DeveloperConsole(), CmdUIOnlyFlag)
{
}

DeveloperConsoleCommand::~DeveloperConsoleCommand()
{
}

void DeveloperConsoleCommand::onExecute(Context* context)
{
  App::instance()->mainWindow()->showDevConsole();
}

Command* CommandFactory::createDeveloperConsoleCommand()
{
  return new DeveloperConsoleCommand;
}

} // namespace app
