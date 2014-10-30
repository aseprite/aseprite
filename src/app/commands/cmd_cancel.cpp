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

#include "app/commands/command.h"

#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"

namespace app {

class CancelCommand : public Command {
public:
  enum Type {
    NoOp,
    All,
  };

  CancelCommand();
  Command* clone() const override { return new CancelCommand(*this); }

protected:
  void onLoadParams(Params* params);
  void onExecute(Context* context);

private:
  Type m_type;
};

CancelCommand::CancelCommand()
  : Command("Cancel",
            "Cancel Current Operation",
            CmdUIOnlyFlag)
  , m_type(NoOp)
{
}

void CancelCommand::onLoadParams(Params* params)
{
  std::string type = params->get("type");
  if (type == "noop") m_type = NoOp;
  else if (type == "all") m_type = All;
}

void CancelCommand::onExecute(Context* context)
{
  switch (m_type) {

    case NoOp:
      // Do nothing.
      break;

    case All:
      if (context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
          ContextFlags::HasVisibleMask)) {
        Command* cmd = CommandsModule::instance()->getCommandByName(CommandId::DeselectMask);
        context->executeCommand(cmd);
      }
      break;
  }
}

Command* CommandFactory::createCancelCommand()
{
  return new CancelCommand;
}

} // namespace app
