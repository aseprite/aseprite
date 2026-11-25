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
#include "app/ui/main_window.h"

namespace app {
using namespace ui;

namespace {
class RecoverFilesCommand final : public Command {
public:
  RecoverFilesCommand();

protected:
  void onExecute(Context* context) override;
  bool onEnabled(Context* context) override;
};

RecoverFilesCommand::RecoverFilesCommand() : Command(CommandId::RecoverFiles())
{
}

bool RecoverFilesCommand::onEnabled(Context* context)
{
#ifdef ENABLE_DATA_RECOVERY
  return context->isUIAvailable();
#endif

  return false;
}

void RecoverFilesCommand::onExecute(Context*)
{
  App::instance()->mainWindow()->showDataRecovery();
}
} // namespace

Command* CommandFactory::createRecoverFilesCommand()
{
  return new RecoverFilesCommand;
}
} // namespace app
