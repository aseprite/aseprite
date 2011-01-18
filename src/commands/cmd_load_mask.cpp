/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "gui/jalert.h"

#include "commands/command.h"
#include "commands/params.h"
#include "dialogs/filesel.h"
#include "modules/gui.h" 
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "sprite_wrappers.h"
#include "util/msk_file.h"

class LoadMaskCommand : public Command
{
  std::string m_filename;

public:
  LoadMaskCommand();
  Command* clone() const { return new LoadMaskCommand(*this); }

protected:
  void onLoadParams(Params* params);

  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LoadMaskCommand::LoadMaskCommand()
  : Command("load_mask",
	    "LoadMask",
	    CmdRecordableFlag)
{
  m_filename = "";
}

void LoadMaskCommand::onLoadParams(Params* params)
{
  m_filename = params->get("filename");
}

bool LoadMaskCommand::onEnabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void LoadMaskCommand::onExecute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  base::string filename = m_filename;

  if (context->is_ui_available()) {
    filename = ase_file_selector("Load .msk File", filename, "msk");
    if (filename.empty())
      return;

    m_filename = filename;
  }

  Mask *mask = load_msk_file(m_filename.c_str());
  if (!mask)
    throw ase_exception("Error loading .msk file: %s",
			static_cast<const char*>(m_filename.c_str()));

  // undo
  if (sprite->getUndo()->isEnabled()) {
    sprite->getUndo()->setLabel("Mask Load");
    sprite->getUndo()->undo_set_mask(sprite);
  }

  sprite->setMask(mask);
  mask_free(mask);

  sprite->generateMaskBoundaries();
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_load_mask_command()
{
  return new LoadMaskCommand;
}
