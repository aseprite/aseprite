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

#include <assert.h>
#include <allegro/debug.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

#include "ui_context.h"
#include "commands/command.h"
#include "commands/params.h"
#include "app.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"

//////////////////////////////////////////////////////////////////////
// select_file

class SelectFileCommand : public Command
{
  gfxobj_id m_sprite_id;

public:
  SelectFileCommand();
  Command* clone() { return new SelectFileCommand(*this); }

protected:
  void load_params(Params* params);
  bool enabled(Context* context);
  bool checked(Context* context);
  void execute(Context* context);
};

SelectFileCommand::SelectFileCommand()
  : Command("select_file",
	    "Select File",
	    CmdUIOnlyFlag)
{
  m_sprite_id = 0;
}

void SelectFileCommand::load_params(Params* params)
{
  if (params->has_param("sprite_id")) {
    m_sprite_id = ustrtol(params->get("sprite_id").c_str(), NULL, 10);
  }
}

bool SelectFileCommand::enabled(Context* context)
{
  /* m_sprite_id != 0, the ID specifies a GfxObj */
  if (m_sprite_id > 0) {
    GfxObj *gfxobj = gfxobj_find(m_sprite_id);
    return gfxobj && gfxobj->type == GFXOBJ_SPRITE;
  }
  /* m_sprite_id=0, means the select "Nothing" option  */
  else
    return true;
}

bool SelectFileCommand::checked(Context* context)
{
  const CurrentSpriteReader sprite(context);

  if (m_sprite_id > 0) {
    GfxObj *gfxobj = gfxobj_find(m_sprite_id);
    return
      gfxobj && gfxobj->type == GFXOBJ_SPRITE &&
      sprite == (Sprite *)gfxobj;
  }
  else
    return sprite == NULL;
}

void SelectFileCommand::execute(Context* context)
{
  if (m_sprite_id > 0) {
    GfxObj* gfxobj = gfxobj_find(m_sprite_id);
    assert(gfxobj != NULL);

    set_sprite_in_more_reliable_editor((Sprite*)gfxobj);
  }
  else {
    set_sprite_in_more_reliable_editor(NULL);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_select_file_command()
{
  return new SelectFileCommand;
}
