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
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/context_access.h"
#include "app/settings/settings.h"
#include "app/tools/tool_box.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui/main_window.h"
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
  return is_tool_loop_brush_image();
}

void DiscardBrushCommand::onExecute(Context* context)
{
  discard_tool_loop_brush_image();

  // Update context bar
  ISettings* settings = UIContext::instance()->settings();
  App::instance()->getMainWindow()->getContextBar()
    ->updateFromTool(settings->getCurrentTool());
}

Command* CommandFactory::createDiscardBrushCommand()
{
  return new DiscardBrushCommand();
}

} // namespace app
