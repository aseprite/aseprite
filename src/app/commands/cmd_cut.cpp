// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
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
#include "app/tx.h"
#include "app/ui/input_chain.h"
#include "app/util/clipboard.h"
#include "app/util/range_utils.h"

namespace app {

class Clipboard;

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
  return App::instance()->isGui() ?
    App::instance()->inputChain().canCut(ctx) : true;
}

void CutCommand::onExecute(Context* ctx)
{
  if (App::instance()->isGui())
    App::instance()->inputChain().cut(ctx);
  else if (ctx->clipboard()) {
    ContextWriter writer(ctx);
    ctx->clipboard()->cut(writer);
  }
}

Command* CommandFactory::createCutCommand()
{
  return new CutCommand;
}

} // namespace app
