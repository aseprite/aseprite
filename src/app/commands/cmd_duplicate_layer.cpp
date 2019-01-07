// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/doc_undo.h"
#include "app/doc_api.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

class DuplicateLayerCommand : public Command {
public:
  DuplicateLayerCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DuplicateLayerCommand::DuplicateLayerCommand()
  : Command(CommandId::DuplicateLayer(), CmdRecordableFlag)
{
}

bool DuplicateLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

void DuplicateLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Doc* document = writer.document();

  {
    Tx tx(writer.context(), "Layer Duplication");
    LayerImage* sourceLayer = static_cast<LayerImage*>(writer.layer());
    DocApi api = document->getApi(tx);
    api.duplicateLayerAfter(sourceLayer,
                            sourceLayer->parent(),
                            sourceLayer);
    tx.commit();
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createDuplicateLayerCommand()
{
  return new DuplicateLayerCommand;
}

} // namespace app
