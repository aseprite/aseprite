// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/tools/tool_box.h"
#include "app/ui/context_bar.h"
#include "app/ui_context.h"
#include "app/util/new_image_from_mask.h"

namespace app {

class DiscardBrushCommand : public Command {
public:
  DiscardBrushCommand();
  Command* clone() const override { return new DiscardBrushCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DiscardBrushCommand::DiscardBrushCommand()
  : Command("DiscardBrush",
            "Discard Brush",
            CmdUIOnlyFlag)
{
}

bool DiscardBrushCommand::onEnabled(Context* context)
{
  ContextBar* ctxBar = App::instance()->contextBar();
  return (ctxBar->activeBrush()->type() == kImageBrushType);
}

void DiscardBrushCommand::onExecute(Context* context)
{
  ContextBar* ctxBar = App::instance()->contextBar();
  ctxBar->discardActiveBrush();
}

Command* CommandFactory::createDiscardBrushCommand()
{
  return new DiscardBrushCommand();
}

} // namespace app
