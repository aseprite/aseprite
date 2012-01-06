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

#include <string>

#include "app.h"
#include "commands/command.h"
#include "commands/params.h"
#include "context.h"
#include "settings/settings.h"
#include "tools/tool.h"

class ChangePenCommand : public Command
{
  enum Change {
    None,
    IncrementSize,
    DecrementSize,
  };

  Change m_change;

public:
  ChangePenCommand();

protected:
  void onLoadParams(Params* params);
  void onExecute(Context* context);
};

ChangePenCommand::ChangePenCommand()
  : Command("ChangePen",
            "Change Pen",
            CmdUIOnlyFlag)
{
  m_change = None;
}

void ChangePenCommand::onLoadParams(Params* params)
{
  std::string change = params->get("change");
  if (change == "increment-size") m_change = IncrementSize;
  else if (change == "decrement-size") m_change = DecrementSize;
}

void ChangePenCommand::onExecute(Context* context)
{
  tools::Tool* current_tool = context->getSettings()->getCurrentTool();
  IToolSettings* tool_settings = context->getSettings()->getToolSettings(current_tool);
  IPenSettings* pen = tool_settings->getPen();

  switch (m_change) {
    case None:
      // Do nothing
      break;
    case IncrementSize:
      if (pen->getSize() < 32)
        pen->setSize(pen->getSize()+1);
      break;
    case DecrementSize:
      if (pen->getSize() > 1)
        pen->setSize(pen->getSize()-1);
      break;
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createChangePenCommand()
{
  return new ChangePenCommand;
}
