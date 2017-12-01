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
#include "app/commands/params.h"
#include "app/context.h"
#include "app/pref/preferences.h"

namespace app {

class SymmetryModeCommand : public Command {
public:
  SymmetryModeCommand();
  Command* clone() const override { return new SymmetryModeCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

SymmetryModeCommand::SymmetryModeCommand()
  : Command("SymmetryMode", CmdUIOnlyFlag)
{
}

bool SymmetryModeCommand::onEnabled(Context* ctx)
{
  return ctx->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                         ContextFlags::HasActiveSprite);
}

bool SymmetryModeCommand::onChecked(Context* ctx)
{
  return Preferences::instance().symmetryMode.enabled();
}

void SymmetryModeCommand::onExecute(Context* ctx)
{
  auto& enabled = Preferences::instance().symmetryMode.enabled;
  enabled(!enabled());
}

Command* CommandFactory::createSymmetryModeCommand()
{
  return new SymmetryModeCommand;
}

} // namespace app
