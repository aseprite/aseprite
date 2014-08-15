/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/ui/main_window.h"
#include "app/ui/workspace.h"
#include "ui/base.h"

namespace app {

class MakeUniqueEditorCommand : public Command {
public:
  MakeUniqueEditorCommand();
  Command* clone() const override { return new MakeUniqueEditorCommand(*this); }

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
  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  if (workspace->activeView() != NULL)
    workspace->makeUnique(workspace->activeView());
}

class SplitEditorHorizontallyCommand : public Command {
public:
  SplitEditorHorizontallyCommand();
  Command* clone() const override { return new SplitEditorHorizontallyCommand(*this); }

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
  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  if (workspace->activeView() != NULL)
    workspace->splitView(workspace->activeView(), JI_HORIZONTAL);
}

class SplitEditorVerticallyCommand : public Command {
public:
  SplitEditorVerticallyCommand();
  Command* clone() const override { return new SplitEditorVerticallyCommand(*this); }

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
  Workspace* workspace = App::instance()->getMainWindow()->getWorkspace();
  if (workspace->activeView() != NULL)
    workspace->splitView(workspace->activeView(), JI_VERTICAL);
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

} // namespace app
