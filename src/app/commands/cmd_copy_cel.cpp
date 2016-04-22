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
#include "app/context_access.h"
#include "app/ui/timeline.h"
#include "ui/base.h"

namespace app {

class CopyCelCommand : public Command {
public:
  CopyCelCommand();
  Command* clone() const override { return new CopyCelCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

CopyCelCommand::CopyCelCommand()
  : Command("CopyCel",
            "Copy Cel",
            CmdUIOnlyFlag)
{
}

bool CopyCelCommand::onEnabled(Context* context)
{
  return App::instance()->timeline()->isMovingCel();
}

void CopyCelCommand::onExecute(Context* context)
{
  App::instance()->timeline()->dropRange(Timeline::kCopy);
}

Command* CommandFactory::createCopyCelCommand()
{
  return new CopyCelCommand;
}

} // namespace app
