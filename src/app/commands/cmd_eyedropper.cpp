/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
#include "raster/image.h"
#include "raster/sprite.h"
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
  Command* clone() const { return new EyedropperCommand(*this); }

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
  Sprite* sprite = editor->getSprite();
  if (!sprite)
    return;

  FrameNumber frame = editor->getFrame();

  // pixel position to get
  int x, y;
  editor->screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);

  // Check if we've to grab alpha channel or the merged color.
  ISettings* settings = UIContext::instance()->settings();
  bool grabAlpha = settings->getGrabAlpha();

  ColorPicker picker;
  picker.pickColor(editor->getDocumentLocation(), x, y,
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
