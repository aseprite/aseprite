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
#include "app/context_access.h"
#include "app/modules/editors.h"
#include "app/ui/editor/editor.h"

namespace app {

class FitScreenCommand : public Command {
public:
  FitScreenCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

FitScreenCommand::FitScreenCommand()
  : Command(CommandId::FitScreen(), CmdUIOnlyFlag)
{
}

bool FitScreenCommand::onEnabled(Context* context)
{
  return (current_editor != nullptr);
}

void FitScreenCommand::onExecute(Context* context)
{
  current_editor->setScrollAndZoomToFitScreen();
}

Command* CommandFactory::createFitScreenCommand()
{
  return new FitScreenCommand;
}

} //namespace app
