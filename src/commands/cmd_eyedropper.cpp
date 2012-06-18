/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include <allegro/unicode.h>

#include "ui/gui.h"

#include "app.h"
#include "commands/command.h"
#include "commands/params.h"
#include "modules/editors.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "widgets/color_bar.h"
#include "widgets/editor/editor.h"

using namespace ui;

//////////////////////////////////////////////////////////////////////
// eyedropper

class EyedropperCommand : public Command
{
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

  // pixel position to get
  int x, y;
  editor->screenToEditor(jmouse_x(0), jmouse_y(0), &x, &y);

  // get the color from the image
  Color color = Color::fromImage(sprite->getPixelFormat(),
                                 sprite->getPixel(x, y));

  // TODO replace the color in the "context", not directly from the color-bar

  // set the color of the color-bar
  if (m_background)
    app_get_colorbar()->setBgColor(color);
  else
    app_get_colorbar()->setFgColor(color);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createEyedropperCommand()
{
  return new EyedropperCommand;
}
