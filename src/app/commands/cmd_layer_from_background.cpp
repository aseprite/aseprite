// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

class LayerFromBackgroundCommand : public Command {
public:
  LayerFromBackgroundCommand();
  Command* clone() const override { return new LayerFromBackgroundCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

LayerFromBackgroundCommand::LayerFromBackgroundCommand()
  : Command("LayerFromBackground",
            "Layer From Background",
            CmdRecordableFlag)
{
}

bool LayerFromBackgroundCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer |
                             ContextFlags::ActiveLayerIsVisible |
                             ContextFlags::ActiveLayerIsEditable |
                             ContextFlags::ActiveLayerIsImage |
                             ContextFlags::ActiveLayerIsBackground) &&
    // Isn't a reference layer
    !context->checkFlags(ContextFlags::ActiveLayerIsReference);
}

void LayerFromBackgroundCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  {
    Transaction transaction(writer.context(), "Layer from Background");
    document->getApi(transaction).layerFromBackground(writer.layer());
    transaction.commit();
  }
  update_screen_for_document(document);
}

Command* CommandFactory::createLayerFromBackgroundCommand()
{
  return new LayerFromBackgroundCommand;
}

} // namespace app
