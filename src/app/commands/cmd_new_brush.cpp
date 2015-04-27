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
#include "doc/mask.h"

namespace app {

class NewBrushCommand : public Command {
public:
  NewBrushCommand();
  Command* clone() const override { return new NewBrushCommand(*this); }

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

NewBrushCommand::NewBrushCommand()
  : Command("NewBrush",
            "New Brush",
            CmdUIOnlyFlag)
{
}

bool NewBrushCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasVisibleMask);
}

void NewBrushCommand::onExecute(Context* context)
{
  // TODO if there is no mask available, create a new Editor state to select the brush bounds

  doc::ImageRef image(new_image_from_mask(UIContext::instance()->activeSite()));
  if (!image) {
    ui::Alert::show("Error<<There is no selected area to create a brush.||&OK");
    return;
  }

  // Set brush
  set_tool_loop_brush_image(
    image, context->activeDocument()->mask()->bounds().getOrigin());

  // Set pencil as current tool
  ISettings* settings = UIContext::instance()->settings();
  tools::Tool* pencil =
    App::instance()->getToolBox()->getToolById(tools::WellKnownTools::Pencil);
  settings->setCurrentTool(pencil);

  // Deselect mask
  Command* cmd =
    CommandsModule::instance()->getCommandByName(CommandId::DeselectMask);
  UIContext::instance()->executeCommand(cmd);
}

Command* CommandFactory::createNewBrushCommand()
{
  return new NewBrushCommand();
}

} // namespace app
