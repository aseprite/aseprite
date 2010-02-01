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

#include "commands/command.h"
#include "modules/editors.h"

//////////////////////////////////////////////////////////////////////
// close_editor

class CloseEditorCommand : public Command
{
public:
  CloseEditorCommand();
  Command* clone() { return new CloseEditorCommand(*this); }

protected:
  void execute(Context* context);
};

CloseEditorCommand::CloseEditorCommand()
  : Command("close_editor",
	    "Close Editor",
	    CmdUIOnlyFlag)
{
}

void CloseEditorCommand::execute(Context* context)
{
  close_editor(current_editor);
}

//////////////////////////////////////////////////////////////////////
// make_unique_editor

class MakeUniqueEditorCommand : public Command
{
public:
  MakeUniqueEditorCommand();
  Command* clone() { return new MakeUniqueEditorCommand(*this); }

protected:
  void execute(Context* context);
};

MakeUniqueEditorCommand::MakeUniqueEditorCommand()
  : Command("make_unique_editor",
	    "Make Unique Editor",
	    CmdUIOnlyFlag)
{
}

void MakeUniqueEditorCommand::execute(Context* context)
{
  make_unique_editor(current_editor);
}

//////////////////////////////////////////////////////////////////////
// split_editor_horizontally

class SplitEditorHorizontallyCommand : public Command
{
public:
  SplitEditorHorizontallyCommand();
  Command* clone() { return new SplitEditorHorizontallyCommand(*this); }

protected:
  void execute(Context* context);
};

SplitEditorHorizontallyCommand::SplitEditorHorizontallyCommand()
  : Command("split_editor_horizontally",
	    "Split Editor Horizontally",
	    CmdUIOnlyFlag)
{
}

void SplitEditorHorizontallyCommand::execute(Context* context)
{
  split_editor(current_editor, JI_HORIZONTAL);
}

//////////////////////////////////////////////////////////////////////
// split_editor_vertically

class SplitEditorVerticallyCommand : public Command
{
public:
  SplitEditorVerticallyCommand();
  Command* clone() { return new SplitEditorVerticallyCommand(*this); }

protected:
  void execute(Context* context);
};

SplitEditorVerticallyCommand::SplitEditorVerticallyCommand()
  : Command("split_editor_vertically",
	    "Split Editor Vertically",
	    CmdUIOnlyFlag)
{
}

void SplitEditorVerticallyCommand::execute(Context* context)
{
  split_editor(current_editor, JI_VERTICAL);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_close_editor_command()
{
  return new CloseEditorCommand;
}

Command* CommandFactory::create_make_unique_editor_command()
{
  return new MakeUniqueEditorCommand;
}

Command* CommandFactory::create_split_editor_horizontally_command()
{
  return new SplitEditorHorizontallyCommand;
}

Command* CommandFactory::create_split_editor_vertically_command()
{
  return new SplitEditorVerticallyCommand;
}
