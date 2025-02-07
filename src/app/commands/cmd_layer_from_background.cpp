// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/layer_from_background.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class LayerFromBackgroundCommand : public Command {
public:
  LayerFromBackgroundCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

LayerFromBackgroundCommand::LayerFromBackgroundCommand()
  : Command(CommandId::LayerFromBackground(), CmdRecordableFlag)
{
}

bool LayerFromBackgroundCommand::onEnabled(Context* context)
{
  return context->checkFlags(
           ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveSprite |
           ContextFlags::HasActiveLayer | ContextFlags::ActiveLayerIsVisible |
           ContextFlags::ActiveLayerIsEditable | ContextFlags::ActiveLayerIsImage |
           ContextFlags::ActiveLayerIsBackground) &&
         // Isn't a reference layer
         !context->checkFlags(ContextFlags::ActiveLayerIsReference);
}

void LayerFromBackgroundCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());
  {
    Tx tx(writer, friendlyName());
    tx(new cmd::LayerFromBackground(writer.layer()));
    tx.commit();
  }
  update_screen_for_document(document);
}

Command* CommandFactory::createLayerFromBackgroundCommand()
{
  return new LayerFromBackgroundCommand;
}

} // namespace app
