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
  void onExecute(Context* context);
};

CloseEditorCommand::CloseEditorCommand()
  : Command("CloseEditor",
            "Close Editor",
            CmdUIOnlyFlag)
{
}

void CloseEditorCommand::onExecute(Context* context)
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
  void onExecute(Context* context);
};

MakeUniqueEditorCommand::MakeUniqueEditorCommand()
  : Command("MakeUniqueEditor",
            "Make Unique Editor",
            CmdUIOnlyFlag)
{
}

void MakeUniqueEditorCommand::onExecute(Context* context)
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
  void onExecute(Context* context);
};

SplitEditorHorizontallyCommand::SplitEditorHorizontallyCommand()
  : Command("SplitEditorHorizontally",
            "Split Editor Horizontally",
            CmdUIOnlyFlag)
{
}

void SplitEditorHorizontallyCommand::onExecute(Context* context)
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
  void onExecute(Context* context);
};

SplitEditorVerticallyCommand::SplitEditorVerticallyCommand()
  : Command("SplitEditorVertically",
            "Split Editor Vertically",
            CmdUIOnlyFlag)
{
}

void SplitEditorVerticallyCommand::onExecute(Context* context)
{
  split_editor(current_editor, JI_VERTICAL);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createCloseEditorCommand()
{
  return new CloseEditorCommand;
}

Command* CommandFactory::createMakeUniqueEditorCommand()
{
  return new MakeUniqueEditorCommand;
}

Command* CommandFactory::createSplitEditorHorizontallyCommand()
{
  return new SplitEditorHorizontallyCommand;
}

Command* CommandFactory::createSplitEditorVerticallyCommand()
{
  return new SplitEditorVerticallyCommand;
}
