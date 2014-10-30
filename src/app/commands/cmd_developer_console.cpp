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
#include "app/ui/devconsole_view.h"
#include "app/ui/main_window.h"
#include "app/ui/workspace.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/window.h"

namespace app {

using namespace ui;

class DeveloperConsoleCommand : public Command {
public:
  DeveloperConsoleCommand();
  ~DeveloperConsoleCommand();

protected:
  void onExecute(Context* context);

  DevConsoleView* m_devConsole;
};

DeveloperConsoleCommand::DeveloperConsoleCommand()
  : Command("DeveloperConsole",
            "Developer Console",
            CmdUIOnlyFlag)
{
  m_devConsole = NULL;
}

DeveloperConsoleCommand::~DeveloperConsoleCommand()
{
  delete m_devConsole;
}

void DeveloperConsoleCommand::onExecute(Context* context)
{
  if (!m_devConsole) {
    m_devConsole = new DevConsoleView();

    App::instance()->getMainWindow()->getWorkspace()->addView(m_devConsole);
  }

  App::instance()->getMainWindow()->getTabsBar()->selectTab(m_devConsole);
  App::instance()->getMainWindow()->getWorkspace()->setActiveView(m_devConsole);
}

Command* CommandFactory::createDeveloperConsoleCommand()
{
  return new DeveloperConsoleCommand;
}

} // namespace app
