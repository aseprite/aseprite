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

#include "base/compiler_specific.h"
#include "commands/command.h"
#include "document.h"
#include "document_wrappers.h"
#include "context.h"
#include "launcher.h"

class OpenWithAppCommand : public Command
{
public:
  OpenWithAppCommand();
  Command* clone() { return new OpenWithAppCommand(*this); }

protected:
  bool onEnabled(Context* context) OVERRIDE;
  void onExecute(Context* context) OVERRIDE;
};

OpenWithAppCommand::OpenWithAppCommand()
  : Command("OpenWithApp",
            "Open With Associated Application",
            CmdUIOnlyFlag)
{
}

bool OpenWithAppCommand::onEnabled(Context* context)
{
  const ActiveDocumentReader document(context);
  return document && document->isAssociatedToFile();
}

void OpenWithAppCommand::onExecute(Context* context)
{
  Launcher::openFile(context->getActiveDocument()->getFilename());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createOpenWithAppCommand()
{
  return new OpenWithAppCommand;
}
