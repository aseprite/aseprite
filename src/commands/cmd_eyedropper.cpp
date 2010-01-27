/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include "jinete/jinete.h"

#include "commands/command.h"
#include "commands/params.h"
#include "app.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "raster/image.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"

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
  void load_params(Params* params);
  void execute(Context* context);
};

EyedropperCommand::EyedropperCommand()
  : Command("eyedropper",
	    "Eyedropper",
	    CmdUIOnlyFlag)
{
  m_background = false;
}

void EyedropperCommand::load_params(Params* params)
{
  std::string target = params->get("target");
  if (target == "foreground") m_background = false;
  else if (target == "background") m_background = true;
}

void EyedropperCommand::execute(Context* context)
{
  JWidget widget = jmanager_get_mouse();
  if (!widget || widget->type != editor_type())
    return;

  Editor* editor = static_cast<Editor*>(widget);
  Sprite* sprite = editor->editor_get_sprite();
  if (!sprite)
    return;

  // pixel position to get
  int x, y;
  editor->screen_to_editor(jmouse_x(0), jmouse_y(0), &x, &y);

  // get the color from the image
  color_t color = color_from_image(sprite->imgtype,
				   sprite_getpixel(sprite, x, y));

  if (color_type(color) != COLOR_TYPE_MASK) {
    // TODO replace the color in the "context", not directly from the color-bar

    // set the color of the color-bar
    if (m_background)
      colorbar_set_bg_color(app_get_colorbar(), color);
    else
      colorbar_set_fg_color(app_get_colorbar(), color);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_eyedropper_command()
{
  return new EyedropperCommand;
}
