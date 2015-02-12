// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"

#include "app/app.h"
#include "app/ui/main_window.h"
#include "app/ui/tabs.h"

namespace app {

class GotoNextTabCommand : public Command {
public:
  GotoNextTabCommand();
  Command* clone() const override { return new GotoNextTabCommand(*this); }

protected:
  void onExecute(Context* context);
};

GotoNextTabCommand::GotoNextTabCommand()
  : Command("GotoNextTab",
            "Go to Next Tab",
            CmdUIOnlyFlag)
{
}

void GotoNextTabCommand::onExecute(Context* context)
{
  App::instance()->getMainWindow()->getTabsBar()->selectNextTab();
}

class GotoPreviousTabCommand : public Command {
public:
  GotoPreviousTabCommand();
  Command* clone() const override { return new GotoPreviousTabCommand(*this); }

protected:
  void onExecute(Context* context);
};

GotoPreviousTabCommand::GotoPreviousTabCommand()
  : Command("GotoPreviousTab",
            "Go to Previous tab",
            CmdRecordableFlag)
{
}

void GotoPreviousTabCommand::onExecute(Context* context)
{
  App::instance()->getMainWindow()->getTabsBar()->selectPreviousTab();
}

Command* CommandFactory::createGotoNextTabCommand()
{
  return new GotoNextTabCommand;
}

Command* CommandFactory::createGotoPreviousTabCommand()
{
  return new GotoPreviousTabCommand;
}

} // namespace app
