// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/transaction.h"
#include "doc/sprite.h"

namespace app {

class FlattenLayersCommand : public Command {
public:
  FlattenLayersCommand();
  Command* clone() const override { return new FlattenLayersCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

FlattenLayersCommand::FlattenLayersCommand()
  : Command("FlattenLayers",
            "Flatten Layers",
            CmdUIOnlyFlag)
{
}

bool FlattenLayersCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void FlattenLayersCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document = writer.document();
  Sprite* sprite = writer.sprite();
  {
    Transaction transaction(writer.context(), "Flatten Layers");
    document->getApi(transaction).flattenLayers(sprite);
    transaction.commit();
  }
  update_screen_for_document(writer.document());
}

Command* CommandFactory::createFlattenLayersCommand()
{
  return new FlattenLayersCommand;
}

} // namespace app
