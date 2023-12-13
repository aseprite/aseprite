// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/background_from_layer.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class BackgroundFromLayerCommand : public Command {
public:
  BackgroundFromLayerCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

BackgroundFromLayerCommand::BackgroundFromLayerCommand()
  : Command(CommandId::BackgroundFromLayer(), CmdRecordableFlag)
{
}

bool BackgroundFromLayerCommand::onEnabled(Context* context)
{
  return
    context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                        ContextFlags::ActiveLayerIsVisible |
                        ContextFlags::ActiveLayerIsEditable |
                        ContextFlags::ActiveLayerIsImage) &&
    // Doesn't have a background layer
    !context->checkFlags(ContextFlags::HasBackgroundLayer) &&
    // Isn't a reference layer
    !context->checkFlags(ContextFlags::ActiveLayerIsReference) &&
    // Isn't a tilemap layer
    // TODO support background tilemaps
    !context->checkFlags(ContextFlags::ActiveLayerIsTilemap);
}

void BackgroundFromLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document(writer.document());

  {
    Tx tx(writer, friendlyName());
    tx(new cmd::BackgroundFromLayer(static_cast<LayerImage*>(writer.layer())));
    tx.commit();
  }

#ifdef ENABLE_UI
  if (context->isUIAvailable())
    update_screen_for_document(document);
#endif
}

Command* CommandFactory::createBackgroundFromLayerCommand()
{
  return new BackgroundFromLayerCommand;
}

} // namespace app
