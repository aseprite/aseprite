// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
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
  : Command("DeveloperConsole",
            "Developer Console",
            CmdUIOnlyFlag)
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
