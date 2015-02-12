// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/document.h"
#include "app/ui/devconsole_view.h"
#include "app/ui/main_window.h"
#include "app/ui/workspace.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/window.h"

namespace app {

using namespace ui;

class DeveloperConsoleCommand : public Command {
public:
  DeveloperConsoleCommand();
  ~DeveloperConsoleCommand();

protected:
  void onExecute(Context* context);

  DevConsoleView* m_devConsole;
};

DeveloperConsoleCommand::DeveloperConsoleCommand()
  : Command("DeveloperConsole",
            "Developer Console",
            CmdUIOnlyFlag)
{
  m_devConsole = NULL;
}

DeveloperConsoleCommand::~DeveloperConsoleCommand()
{
  delete m_devConsole;
}

void DeveloperConsoleCommand::onExecute(Context* context)
{
  if (!m_devConsole) {
    m_devConsole = new DevConsoleView();

    App::instance()->getMainWindow()->getWorkspace()->addView(m_devConsole);
  }

  App::instance()->getMainWindow()->getTabsBar()->selectTab(m_devConsole);
  App::instance()->getMainWindow()->getWorkspace()->setActiveView(m_devConsole);
}

Command* CommandFactory::createDeveloperConsoleCommand()
{
  return new DeveloperConsoleCommand;
}

} // namespace app
