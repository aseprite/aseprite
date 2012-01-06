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

#include "gui/gui.h"

#include "commands/command.h"
#include "dialogs/aniedit.h"
#include "document_wrappers.h"

//////////////////////////////////////////////////////////////////////
// film_editor

class FilmEditorCommand : public Command
{
public:
  FilmEditorCommand();
  Command* clone() { return new FilmEditorCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

FilmEditorCommand::FilmEditorCommand()
  : Command("FilmEditor",
            "Animation Editor",
            CmdUIOnlyFlag)
{
}

bool FilmEditorCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void FilmEditorCommand::onExecute(Context* context)
{
  switch_between_animation_and_sprite_editor();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createFilmEditorCommand()
{
  return new FilmEditorCommand;
}
