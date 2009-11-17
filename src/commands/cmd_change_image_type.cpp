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

#include "commands/command.h"
#include "commands/params.h"
#include "core/app.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "raster/image.h"
#include "raster/quant.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "undoable.h"

class ChangeImageTypeCommand : public Command
{
  int m_imgtype;
  bool m_dithering;
public:
  ChangeImageTypeCommand();
  Command* clone() const { return new ChangeImageTypeCommand(*this); }

protected:
  void load_params(Params* params);
  bool enabled(Context* context);
  bool checked(Context* context);
  void execute(Context* context);
};

ChangeImageTypeCommand::ChangeImageTypeCommand()
  : Command("change_image_type",
	    "Change Image Type",
	    CmdUIOnlyFlag)
{
  m_imgtype = IMAGE_RGB;
  m_dithering = DITHERING_NONE;
}

void ChangeImageTypeCommand::load_params(Params* params)
{
  std::string imgtype = params->get("imgtype");
  if (imgtype == "rgb") m_imgtype = IMAGE_RGB;
  else if (imgtype == "grayscale") m_imgtype = IMAGE_GRAYSCALE;
  else if (imgtype == "indexed") m_imgtype = IMAGE_INDEXED;

  std::string dithering = params->get("dithering");
  if (dithering == "ordered")
    m_dithering = DITHERING_ORDERED;
  else
    m_dithering = DITHERING_NONE;
}

bool ChangeImageTypeCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);

  if (sprite != NULL) {
    if (sprite->imgtype == IMAGE_INDEXED && m_imgtype == IMAGE_INDEXED && m_dithering == DITHERING_NONE)
      return false;
  }

  return true;
}

bool ChangeImageTypeCommand::checked(Context* context)
{
  const CurrentSpriteReader sprite(context);

  if (sprite != NULL) {
    if (sprite->imgtype == IMAGE_INDEXED && m_imgtype == IMAGE_INDEXED && m_dithering == DITHERING_NONE)
      return false;
  }

  return
    sprite != NULL &&
    sprite->imgtype == m_imgtype;
}

void ChangeImageTypeCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);
  {
    CurrentSpriteRgbMap rgbmap;
    Undoable undoable(sprite, "Color Mode Change");
    undoable.set_imgtype(m_imgtype, m_dithering);
    undoable.commit();
  }
  app_refresh_screen(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_change_image_type_command()
{
  return new ChangeImageTypeCommand;
}
