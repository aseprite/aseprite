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

class CutCommand : public Command {
public:
  CutCommand();
  Command* clone() const override { return new CutCommand(*this); }

protected:
  bool onEnabled(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

CutCommand::CutCommand()
  : Command("Cut",
            "Cut",
            CmdUIOnlyFlag)
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
