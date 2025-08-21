// Aseprite
// Copyright (c) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/ui/layout_selector.h"
#include "app/ui/main_window.h"

namespace app {

class ToggleWorkspaceLayoutCommand : public Command {
public:
  ToggleWorkspaceLayoutCommand();

protected:
  bool onEnabled(Context* ctx) override;
  bool onChecked(Context* ctx) override;
  void onExecute(Context* ctx) override;
};

ToggleWorkspaceLayoutCommand::ToggleWorkspaceLayoutCommand()
  : Command(CommandId::ToggleWorkspaceLayout())
{
}

bool ToggleWorkspaceLayoutCommand::onEnabled(Context* ctx)
{
  return ctx->isUIAvailable();
}

bool ToggleWorkspaceLayoutCommand::onChecked(Context* ctx)
{
  return App::instance()->mainWindow()->layoutSelector()->isSelectorVisible();
}

void ToggleWorkspaceLayoutCommand::onExecute(Context* ctx)
{
  App::instance()->mainWindow()->layoutSelector()->switchSelectorFromCommand();
}

Command* CommandFactory::createToggleWorkspaceLayoutCommand()
{
  return new ToggleWorkspaceLayoutCommand();
}

} // namespace app
