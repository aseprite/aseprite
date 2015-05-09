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
#include "app/ui/input_chain.h"

namespace app {

class ClearCommand : public Command {
public:
  ClearCommand();
  Command* clone() const override { return new ClearCommand(*this); }

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

ClearCommand::ClearCommand()
  : Command("Clear",
            "Clear",
            CmdUIOnlyFlag)
{
}

bool ClearCommand::onEnabled(Context* ctx)
{
  return App::instance()->inputChain().canClear(ctx);
}

void ClearCommand::onExecute(Context* ctx)
{
  App::instance()->inputChain().clear(ctx);
}

Command* CommandFactory::createClearCommand()
{
  return new ClearCommand;
}

} // namespace app
