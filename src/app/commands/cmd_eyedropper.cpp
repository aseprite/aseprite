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
#include "app/commands/params.h"
#include "app/document_location.h"
#include "app/modules/editors.h"
#include "app/settings/settings.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui_context.h"
#include "doc/image.h"
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
  void onLoadParams(Params* params);
  void onExecute(Context* context);
};

EyedropperCommand::EyedropperCommand()
  : Command("Eyedropper",
            "Eyedropper",
            CmdUIOnlyFlag)
{
  m_background = false;
}

void EyedropperCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
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

  // pixel position to get
  gfx::Point pixelPos = editor->screenToEditor(ui::get_mouse_position());

  // Check if we've to grab alpha channel or the merged color.
  ISettings* settings = UIContext::instance()->settings();
  bool grabAlpha = settings->getGrabAlpha();

  ColorPicker picker;
  picker.pickColor(editor->getDocumentLocation(),
    pixelPos,
    grabAlpha ?
    ColorPicker::FromActiveLayer:
    ColorPicker::FromComposition);

  if (grabAlpha) {
    tools::ToolBox* toolBox = App::instance()->getToolBox();
    for (tools::ToolIterator it=toolBox->begin(), end=toolBox->end(); it!=end; ++it) {
      settings->getToolSettings(*it)->setOpacity(picker.alpha());
    }
  }

  if (m_background)
    settings->setBgColor(picker.color());
  else
    settings->setFgColor(picker.color());
}

Command* CommandFactory::createEyedropperCommand()
{
  return new EyedropperCommand;
}

} // namespace app
