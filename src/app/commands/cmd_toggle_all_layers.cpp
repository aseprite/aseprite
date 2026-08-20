// Aseprite
// Copyright (C) 2026  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/toggle_all_layers.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/transaction.h"
#include "doc/sprite.h"

namespace app {

class ToggleAllLayersCommand : public Command {
public:
  ToggleAllLayersCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

ToggleAllLayersCommand::ToggleAllLayersCommand() : Command(CommandId::ToggleAllLayers())
{
}

bool ToggleAllLayersCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}
void ToggleAllLayersCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  app::Doc* doc = writer.document();
  doc::Sprite* sprite = writer.sprite();

  if (!doc || !sprite)
    return;

  bool isCurrentlyVisible = true;
  bool newState = !isCurrentlyVisible;
  Transaction transaction(writer.context(), doc, "Toggle All Layers");
  transaction.execute(new app::cmd::ToggleAllLayers(sprite, newState));
  transaction.commit();
}

Command* CommandFactory::createToggleAllLayersCommand()
{
  return new ToggleAllLayersCommand;
}

} // namespace app
