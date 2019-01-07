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

class CutCommand : public Command {
public:
  CutCommand();

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

CutCommand::CutCommand()
  : Command(CommandId::Cut(), CmdUIOnlyFlag)
{
}

bool CutCommand::onEnabled(Context* ctx)
{
  return App::instance()->inputChain().canCut(ctx);
}

void CutCommand::onExecute(Context* ctx)
{
  App::instance()->inputChain().cut(ctx);
}

Command* CommandFactory::createCutCommand()
{
  return new CutCommand;
}

} // namespace app
