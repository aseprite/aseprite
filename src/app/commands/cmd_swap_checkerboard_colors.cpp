// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/ui/editor/editor.h"
#include "ui/base.h"

namespace app {

class SwapCheckerboardColorsCommand : public Command {
public:
  SwapCheckerboardColorsCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

SwapCheckerboardColorsCommand::SwapCheckerboardColorsCommand()
  : Command(CommandId::SwapCheckerboardColors(), CmdUIOnlyFlag)
{
}

bool SwapCheckerboardColorsCommand::onEnabled(Context* context)
{
  return true;
}

void SwapCheckerboardColorsCommand::onExecute(Context* context)
{
  DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
  app::Color c1 = docPref.bg.color1();
  app::Color c2 = docPref.bg.color2();

  docPref.bg.color1(c2);
  docPref.bg.color2(c1);
}
Command* CommandFactory::createSwapCheckerboardColorsCommand()
{
  return new SwapCheckerboardColorsCommand;
}

} // namespace app
