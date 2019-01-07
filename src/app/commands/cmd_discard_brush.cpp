// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

DiscardBrushCommand::DiscardBrushCommand()
  : Command(CommandId::DiscardBrush(), CmdUIOnlyFlag)
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
