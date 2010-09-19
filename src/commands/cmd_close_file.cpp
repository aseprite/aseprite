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

#include <memory>
#include <allegro.h>
#include "jinete/jinete.h"

#include "commands/command.h"
#include "commands/commands.h"
#include "app.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "sprite_wrappers.h"
#include "ui_context.h"
#include "widgets/statebar.h"

static bool close_current_sprite(Context* context);

//////////////////////////////////////////////////////////////////////
// close_file

class CloseFileCommand : public Command
{
public:
  CloseFileCommand()
    : Command("close_file",
	      "Close File",
	      CmdUIOnlyFlag)
  {
  }

  Command* clone() const { return new CloseFileCommand(*this); }

protected:

  bool onEnabled(Context* context)
  {
    const CurrentSpriteReader sprite(context);
    return sprite != NULL;
  }

  void onExecute(Context* context)
  {
    close_current_sprite(context);
  }

private:
  static char* read_authors_txt(const char *filename);
};

//////////////////////////////////////////////////////////////////////
// close_all_files

class CloseAllFilesCommand : public Command
{
public:
  CloseAllFilesCommand()
    : Command("close_all_files",
	      "Close All Files",
	      CmdRecordableFlag)
  {
  }

  Command* clone() const { return new CloseAllFilesCommand(*this); }

protected:

  bool onEnabled(Context* context)
  {
    return !context->get_sprite_list().empty();
  }

  void onExecute(Context* context)
  {
    if (!context->get_current_sprite())
      set_sprite_in_more_reliable_editor(context->get_first_sprite());

    while (true) {
      if (context->get_current_sprite() != NULL) {
	if (!close_current_sprite(context))
	  break;
      }
      else
	break;
    }
  }

};

//////////////////////////////////////////////////////////////////////

/**
 * Closes the current sprite, asking to the user if to save it if it's
 * modified.
 */
static bool close_current_sprite(Context* context)
{
  bool save_it;

try_again:;
  // This flag indicates if we have to sabe the sprite before to destroy it
  save_it = false;
  {
    // The sprite is locked as reader temporaly
    CurrentSpriteReader sprite(context);

    // see if the sprite has changes
    while (sprite->isModified()) {
      // ask what want to do the user with the changes in the sprite
      int ret = jalert("Warning<<Saving changes in:<<%s||&Save||Do&n't Save||&Cancel",
		       get_filename(sprite->getFilename()));

      if (ret == 1) {
	// "save": save the changes
	save_it = true;
	break;
      }
      else if (ret != 2) {
	// "cancel" or "ESC" */
	return false; // we back doing nothing
      }
      else {
	// "discard"
	break;
      }
    }
  }

  // Does we need to save the sprite?
  if (save_it) {
    Command* save_command =
      CommandsModule::instance()->get_command_by_name(CommandId::save_file);
    context->execute_command(save_command);

    goto try_again;
  }

  // Destroy the sprite (locking it as writer)
  {
    CurrentSpriteWriter sprite(context);
    app_get_statusbar()
      ->setStatusText(0, "Sprite '%s' closed.",
		      get_filename(sprite->getFilename()));
    sprite.destroy();
  }
  return true;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_close_file_command()
{
  return new CloseFileCommand;
}

Command* CommandFactory::create_close_all_files_command()
{
  return new CloseAllFilesCommand;
}
