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

namespace app {

class GotoNextTabCommand : public Command {
public:
  GotoNextTabCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

GotoNextTabCommand::GotoNextTabCommand()
  : Command(CommandId::GotoNextTab(), CmdUIOnlyFlag)
{
}

bool GotoNextTabCommand::onEnabled(Context* context)
{
  return App::instance()->workspace()->canSelectOtherTab();
}

void GotoNextTabCommand::onExecute(Context* context)
{
  App::instance()->workspace()->selectNextTab();
}

class GotoPreviousTabCommand : public Command {
public:
  GotoPreviousTabCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

GotoPreviousTabCommand::GotoPreviousTabCommand()
  : Command(CommandId::GotoPreviousTab(), CmdRecordableFlag)
{
}

bool GotoPreviousTabCommand::onEnabled(Context* context)
{
  return App::instance()->workspace()->canSelectOtherTab();
}

void GotoPreviousTabCommand::onExecute(Context* context)
{
  App::instance()->workspace()->selectPreviousTab();
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
