// Aseprite
// Copyright (C) 2001-2025  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "doc/layer.h"

namespace app {

class DuplicateLayerCommand : public Command {
public:
  DuplicateLayerCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DuplicateLayerCommand::DuplicateLayerCommand() : Command(CommandId::DuplicateLayer())
{
}

bool DuplicateLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable | ContextFlags::HasActiveLayer);
}

void DuplicateLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document = writer.document();

  {
    Tx tx(writer, "Layer Duplication");
    LayerImage* sourceLayer = static_cast<LayerImage*>(writer.layer());
    DocApi api = document->getApi(tx);
    api.duplicateLayerAfter(sourceLayer, sourceLayer->parent(), sourceLayer);
    tx.commit();
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createDuplicateLayerCommand()
{
  return new DuplicateLayerCommand;
}

} // namespace app
