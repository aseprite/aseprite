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
  return App::instance()->inputChain().canPaste(ctx);
}

void PasteCommand::onExecute(Context* ctx)
{
  App::instance()->inputChain().paste(ctx);
}

Command* CommandFactory::createPasteCommand()
{
  return new PasteCommand;
}

} // namespace app
