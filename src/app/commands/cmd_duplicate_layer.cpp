// Aseprite
// Copyright (C) 2001-2015  David Capello
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
#include "app/document_api.h"
#include "app/document_undo.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "app/transaction.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "ui/ui.h"

namespace app {

class DuplicateLayerCommand : public Command {
public:
  DuplicateLayerCommand();
  Command* clone() const override { return new DuplicateLayerCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DuplicateLayerCommand::DuplicateLayerCommand()
  : Command("DuplicateLayer",
            "Duplicate Layer",
            CmdRecordableFlag)
{
}

bool DuplicateLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer |
                             ContextFlags::ActiveLayerIsImage);
}

void DuplicateLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document = writer.document();

  {
    Transaction transaction(writer.context(), "Layer Duplication");
    LayerImage* sourceLayer = static_cast<LayerImage*>(writer.layer());
    DocumentApi api = document->getApi(transaction);
    api.duplicateLayerAfter(sourceLayer, sourceLayer);
    transaction.commit();
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createDuplicateLayerCommand()
{
  return new DuplicateLayerCommand;
}

} // namespace app
