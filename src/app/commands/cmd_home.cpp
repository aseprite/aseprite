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
#include "app/context.h"
#include "app/ui/main_window.h"

namespace app {

using namespace ui;

class HomeCommand : public Command {
public:
  HomeCommand();
  ~HomeCommand();

protected:
  void onExecute(Context* context) override;
  bool onEnabled(Context* context) override;
};

HomeCommand::HomeCommand() : Command(CommandId::Home())
{
}

HomeCommand::~HomeCommand()
{
}

void HomeCommand::onExecute(Context* context)
{
  App::instance()->mainWindow()->showHome();
}

bool HomeCommand::onEnabled(Context* context)
{
  return context->isUIAvailable() && !App::instance()->mainWindow()->isHomeSelected();
}

Command* CommandFactory::createHomeCommand()
{
  return new HomeCommand;
}

} // namespace app
