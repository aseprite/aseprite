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

#include <string>

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/settings/settings.h"
#include "app/tools/tool.h"

namespace app {

class ChangeBrushCommand : public Command {
  enum Change {
    None,
    IncrementSize,
    DecrementSize,
    IncrementAngle,
    DecrementAngle,
  };

  Change m_change;

public:
  ChangeBrushCommand();

protected:
  void onLoadParams(Params* params);
  void onExecute(Context* context);
};

ChangeBrushCommand::ChangeBrushCommand()
  : Command("ChangeBrush",
            "Change Brush",
            CmdUIOnlyFlag)
{
  m_change = None;
}

void ChangeBrushCommand::onLoadParams(Params* params)
{
  std::string change = params->get("change");
  if (change == "increment-size") m_change = IncrementSize;
  else if (change == "decrement-size") m_change = DecrementSize;
  else if (change == "increment-angle") m_change = IncrementAngle;
  else if (change == "decrement-angle") m_change = DecrementAngle;
}

void ChangeBrushCommand::onExecute(Context* context)
{
  tools::Tool* current_tool = context->settings()->getCurrentTool();
  IToolSettings* tool_settings = context->settings()->getToolSettings(current_tool);
  IBrushSettings* brush = tool_settings->getBrush();

  switch (m_change) {
    case None:
      // Do nothing
      break;
    case IncrementSize:
      if (brush->getSize() < 32)
        brush->setSize(brush->getSize()+1);
      break;
    case DecrementSize:
      if (brush->getSize() > 1)
        brush->setSize(brush->getSize()-1);
      break;
    case IncrementAngle:
      if (brush->getAngle() < 180)
        brush->setAngle(brush->getAngle()+1);
      break;
    case DecrementAngle:
      if (brush->getAngle() > 0)
        brush->setAngle(brush->getAngle()-1);
      break;
  }
}

Command* CommandFactory::createChangeBrushCommand()
{
  return new ChangeBrushCommand;
}

} // namespace app
