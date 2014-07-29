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
#include "app/context.h"
#include "app/document.h"
#include "app/ui/main_window.h"
#include "ui/alert.h"

namespace app {

class ExitCommand : public Command {
public:
  ExitCommand();
  Command* clone() const OVERRIDE { return new ExitCommand(*this); }

protected:
  void onExecute(Context* context);
};

ExitCommand::ExitCommand()
  : Command("Exit",
            "Exit",
            CmdUIOnlyFlag)
{
}

void ExitCommand::onExecute(Context* context)
{
  const doc::Documents& docs = context->documents();
  bool modifiedFiles = false;

  for (doc::Documents::const_iterator it=docs.begin(), end=docs.end(); it!=end; ++it) {
    const Document* document = static_cast<Document*>(*it);
    if (document->isModified()) {
      modifiedFiles = true;
      break;
    }
  }

  if (modifiedFiles) {
    if (ui::Alert::show("Warning<<There are sprites with changes.<<Do you want quit anyway?||&Yes||&No") != 1)
      return; // In this case the user doesn't want to close with modified files
  }

  // Close the window
  App::instance()->getMainWindow()->closeWindow(NULL);
}

Command* CommandFactory::createExitCommand()
{
  return new ExitCommand;
}

} // namespace app
