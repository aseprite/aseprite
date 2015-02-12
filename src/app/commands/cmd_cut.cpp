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
#include "app/util/clipboard.h"
#include "app/util/misc.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/base.h"

namespace app {

class CutCommand : public Command {
public:
  CutCommand();
  Command* clone() const override { return new CutCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CutCommand::CutCommand()
  : Command("Cut",
            "Cut",
            CmdUIOnlyFlag)
{
}

bool CutCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsVisible |
                             ContextFlags::ActiveLayerIsEditable |
                             ContextFlags::HasVisibleMask |
                             ContextFlags::HasActiveImage);
}

void CutCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  clipboard::cut(writer);
}

Command* CommandFactory::createCutCommand()
{
  return new CutCommand;
}

} // namespace app
