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
#include "app/context.h"
#include "app/ui/input_chain.h"
#include "app/util/clipboard.h"

namespace app {

class PasteCommand : public Command {
public:
  PasteCommand();

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

PasteCommand::PasteCommand()
  : Command(CommandId::Paste(), CmdUIOnlyFlag)
{
}

bool PasteCommand::onEnabled(Context* ctx)
{
  return ctx->isUIAvailable() ?
    App::instance()->inputChain().canPaste(ctx) :
    ctx->clipboard()->isImageAvailable();
}

void PasteCommand::onExecute(Context* ctx)
{
  if (ctx->isUIAvailable())
    App::instance()->inputChain().paste(ctx);
  else if (ctx->clipboard())
    ctx->clipboard()->paste(ctx, false);
}

Command* CommandFactory::createPasteCommand()
{
  return new PasteCommand;
}

} // namespace app
