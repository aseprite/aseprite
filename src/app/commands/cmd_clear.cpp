// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/deselect_mask.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/transaction.h"
#include "app/ui/color_bar.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "doc/layer.h"
#include "doc/mask.h"

namespace app {

class ClearCommand : public Command {
public:
  ClearCommand();
  Command* clone() const override { return new ClearCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ClearCommand::ClearCommand()
  : Command("Clear",
            "Clear",
            CmdUIOnlyFlag)
{
}

bool ClearCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsVisible |
                             ContextFlags::ActiveLayerIsEditable |
                             ContextFlags::ActiveLayerIsImage);
}

void ClearCommand::onExecute(Context* context)
{
  // Clear of several frames is handled with ClearCel command.
  DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
  if (range.enabled()) {
    Command* subCommand = NULL;

    switch (range.type()) {
      case DocumentRange::kCels:
        subCommand = CommandsModule::instance()->getCommandByName(CommandId::ClearCel);
        break;
      case DocumentRange::kFrames:
        subCommand = CommandsModule::instance()->getCommandByName(CommandId::RemoveFrame);
        break;
      case DocumentRange::kLayers:
        subCommand = CommandsModule::instance()->getCommandByName(CommandId::RemoveLayer);
        break;
    }

    if (subCommand) {
      context->executeCommand(subCommand);
      return;
    }
  }

  // TODO add support to clear the mask in the selected range of frames.

  ContextWriter writer(context);
  Document* document = writer.document();
  bool visibleMask = document->isMaskVisible();

  if (!writer.cel())
    return;

  {
    Transaction transaction(writer.context(), "Clear");
    transaction.execute(new cmd::ClearMask(writer.cel()));
    if (visibleMask)
      transaction.execute(new cmd::DeselectMask(document));
    transaction.commit();
  }
  if (visibleMask)
    document->generateMaskBoundaries();
  update_screen_for_document(document);
}

Command* CommandFactory::createClearCommand()
{
  return new ClearCommand;
}

} // namespace app
