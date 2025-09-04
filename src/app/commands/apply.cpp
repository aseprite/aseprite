// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context.h"
#include "app/ui/editor/editor.h"

namespace app {

// Depends on the current context/state, used to apply the current
// transformation (drop pixels).
class ApplyCommand : public Command {
public:
  ApplyCommand();

protected:
  void onExecute(Context* ctx) override;
};

ApplyCommand::ApplyCommand() : Command(CommandId::Apply())
{
}

void ApplyCommand::onExecute(Context* ctx)
{
  if (!ctx->isUIAvailable())
    return;

  auto* editor = Editor::activeEditor();
  if (editor && editor->isMovingPixels())
    editor->dropMovingPixels();
}

Command* CommandFactory::createApplyCommand()
{
  return new ApplyCommand;
}

} // namespace app
