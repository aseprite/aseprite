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
#include "app/context_access.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "ui/base.h"

namespace app {

class MoveCelCommand : public Command {
public:
  MoveCelCommand();
  Command* clone() const override { return new MoveCelCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

MoveCelCommand::MoveCelCommand()
  : Command("MoveCel",
            "Move Cel",
            CmdUIOnlyFlag)
{
}

bool MoveCelCommand::onEnabled(Context* context)
{
  return App::instance()->getMainWindow()->getTimeline()->isMovingCel();
}

void MoveCelCommand::onExecute(Context* context)
{
  App::instance()->getMainWindow()->getTimeline()->dropRange(Timeline::kMove);
}

Command* CommandFactory::createMoveCelCommand()
{
  return new MoveCelCommand;
}

} // namespace app
