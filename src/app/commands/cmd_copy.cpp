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
#include "app/ui/input_chain.h"

namespace app {

class CopyCommand : public Command {
public:
  CopyCommand();

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

CopyCommand::CopyCommand()
  : Command(CommandId::Copy(), CmdUIOnlyFlag)
{
}

bool CopyCommand::onEnabled(Context* ctx)
{
  return App::instance()->inputChain().canCopy(ctx);
}

void CopyCommand::onExecute(Context* ctx)
{
  App::instance()->inputChain().copy(ctx);
}

Command* CommandFactory::createCopyCommand()
{
  return new CopyCommand;
}

} // namespace app
