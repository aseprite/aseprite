// Aseprite
// Copyright (C) 2020-2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/ui.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context.h"
#include "os/system.h"
#include "os/window.h"

namespace app {

class FullscreenModeCommand : public Command {
public:
  FullscreenModeCommand();

protected:
  void onExecute(Context* context) override;
};

FullscreenModeCommand::FullscreenModeCommand() : Command(CommandId::FullscreenMode(), CmdUIOnlyFlag)
{
}

// Shows the sprite using the complete screen.
void FullscreenModeCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  ui::Manager* manager = ui::Manager::getDefault();
  ASSERT(manager);
  if (!manager)
    return;

  os::Window* window = manager->display()->nativeWindow();
  ASSERT(window);
  if (!window)
    return;

  window->setFullscreen(!window->isFullscreen());
}

Command* CommandFactory::createFullscreenModeCommand()
{
  return new FullscreenModeCommand;
}

} // namespace app
