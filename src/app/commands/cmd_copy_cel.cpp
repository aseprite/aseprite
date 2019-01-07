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
#include "app/ui/timeline/timeline.h"
#include "ui/base.h"

namespace app {

class CopyCelCommand : public Command {
public:
  CopyCelCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

CopyCelCommand::CopyCelCommand()
  : Command(CommandId::CopyCel(), CmdUIOnlyFlag)
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
