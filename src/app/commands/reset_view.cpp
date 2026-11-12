// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/ui/editor/editor.h"

namespace app {

class ResetViewCommand : public Command {
public:
  ResetViewCommand();

protected:
  void onExecute(Context* ctx) override;
};

ResetViewCommand::ResetViewCommand() : Command(CommandId::ResetView())
{
}

void ResetViewCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  if (auto* editor = Editor::activeEditor())
    editor->setViewOptions(ViewOptions());
}

Command* CommandFactory::createResetViewCommand()
{
  return new ResetViewCommand;
}

} // namespace app
