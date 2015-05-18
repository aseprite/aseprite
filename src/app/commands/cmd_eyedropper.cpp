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
#include "app/color.h"
#include "app/color_picker.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/modules/editors.h"
#include "app/pref/preferences.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/image.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "ui/manager.h"
#include "ui/system.h"

namespace app {

using namespace ui;

class EyedropperCommand : public Command {
  /**
   * True means "pick background color", false the foreground color.
   */
  bool m_background;

public:
  EyedropperCommand();
  Command* clone() const override { return new EyedropperCommand(*this); }

protected:
  void onLoadParams(const Params& params) override;
  void onExecute(Context* context) override;
};

EyedropperCommand::EyedropperCommand()
  : Command("Eyedropper",
            "Eyedropper",
            CmdUIOnlyFlag)
{
  m_background = false;
}

void EyedropperCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;
}

void EyedropperCommand::onExecute(Context* context)
{
  Widget* widget = ui::Manager::getDefault()->getMouse();
  if (!widget || widget->type != editor_type())
    return;

  Editor* editor = static_cast<Editor*>(widget);
  Sprite* sprite = editor->sprite();
  if (!sprite)
    return;

  // Discard current image brush
  {
    Command* discardBrush = CommandsModule::instance()->getCommandByName(CommandId::DiscardBrush);
    context->executeCommand(discardBrush);
  }

  // Pixel position to get
  gfx::Point pixelPos = editor->screenToEditor(ui::get_mouse_position());

  // Check if we've to grab alpha channel or the merged color.
  Preferences& pref = Preferences::instance();
  bool grabAlpha = pref.editor.grabAlpha();

  ColorPicker picker;
  picker.pickColor(editor->getSite(),
    pixelPos,
    grabAlpha ?
    ColorPicker::FromActiveLayer:
    ColorPicker::FromComposition);

  if (grabAlpha) {
    tools::ToolBox* toolBox = App::instance()->getToolBox();
    for (auto tool : *toolBox)
      pref.tool(tool).opacity(picker.alpha());
  }

  if (m_background)
    pref.colorBar.bgColor(picker.color());
  else
    pref.colorBar.fgColor(picker.color());
}

Command* CommandFactory::createEyedropperCommand()
{
  return new EyedropperCommand;
}

} // namespace app
