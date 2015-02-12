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
#include "app/context.h"
#include "app/util/clipboard.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class PasteCommand : public Command {
public:
  PasteCommand();
  Command* clone() const override { return new PasteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

PasteCommand::PasteCommand()
  : Command("Paste",
            "Paste",
            CmdUIOnlyFlag)
{
}

bool PasteCommand::onEnabled(Context* context)
{
  return
    (clipboard::get_current_format() == clipboard::ClipboardImage &&
      context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
        ContextFlags::ActiveLayerIsVisible |
        ContextFlags::ActiveLayerIsEditable |
        ContextFlags::ActiveLayerIsImage)) ||
    (clipboard::get_current_format() == clipboard::ClipboardDocumentRange &&
      context->checkFlags(ContextFlags::ActiveDocumentIsWritable));
}

void PasteCommand::onExecute(Context* context)
{
  clipboard::paste();
}

Command* CommandFactory::createPasteCommand()
{
  return new PasteCommand;
}

} // namespace app
