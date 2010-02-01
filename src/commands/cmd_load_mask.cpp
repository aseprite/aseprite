/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "jinete/jalert.h"

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
  void load_params(Params* params);

  bool enabled(Context* context);
  void execute(Context* context);
};

LoadMaskCommand::LoadMaskCommand()
  : Command("load_mask",
	    "LoadMask",
	    CmdRecordableFlag)
{
  m_filename = "";
}

void LoadMaskCommand::load_params(Params* params)
{
  m_filename = params->get("filename");
}

bool LoadMaskCommand::enabled(Context* context)
{
  const CurrentSpriteReader sprite(context);
  return sprite != NULL;
}

void LoadMaskCommand::execute(Context* context)
{
  CurrentSpriteWriter sprite(context);

  jstring filename = m_filename;

  if (context->is_ui_available()) {
    filename = ase_file_selector(_("Load .msk File"), filename, "msk");
    if (filename.empty())
      return;

    m_filename = filename;
  }

  Mask *mask = load_msk_file(m_filename.c_str());
  if (!mask)
    throw ase_exception("Error loading .msk file: " + m_filename);

  // undo
  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "Mask Load");
    undo_set_mask(sprite->undo, sprite);
  }

  sprite_set_mask(sprite, mask);
  mask_free(mask);

  sprite_generate_mask_boundaries(sprite);
  update_screen_for_sprite(sprite);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_load_mask_command()
{
  return new LoadMaskCommand;
}
