// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/ui/floating_toolbox.h"
#include "app/ui/main_window.h"

namespace app {

class FloatingToolboxCommand : public Command {
public:
  FloatingToolboxCommand();

protected:
  bool onEnabled(Context* context) override;
  bool onChecked(Context* context) override;
  void onExecute(Context* context) override;
};

FloatingToolboxCommand::FloatingToolboxCommand() : Command(CommandId::FloatingToolbox())
{
}

bool FloatingToolboxCommand::onEnabled(Context* context)
{
  return context->isUIAvailable();
}

bool FloatingToolboxCommand::onChecked(Context* context)
{
  MainWindow* mainWin = App::instance()->mainWindow();
  if (!mainWin)
    return false;

  FloatingToolbox* ft = mainWin->getFloatingToolbox();
  return (ft && ft->isVisible());
}

void FloatingToolboxCommand::onExecute(Context* context)
{
  FloatingToolbox* ft = App::instance()->mainWindow()->getFloatingToolbox();

  bool state = ft->isEnabled();
  ft->setEnabled(!state);
}

Command* CommandFactory::createFloatingToolboxCommand()
{
  return new FloatingToolboxCommand;
}

} // namespace app
