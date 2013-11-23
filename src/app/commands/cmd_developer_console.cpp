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
#include "app/context.h"
#include "app/document.h"
#include "app/documents.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/window.h"

namespace app {

using namespace ui;

class DeveloperConsole : public Window {
public:
  DeveloperConsole()
    : Window(WithTitleBar, "Developer Console")
    , m_vbox(JI_VERTICAL)
  {
    m_vbox.addChild(&m_docs);
    addChild(&m_vbox);

    remapWindow();
    centerWindow();
  }

  void updateDocuments(Context* context)
  {
    m_docs.removeAllItems();
    m_docs.addItem("Documents");
    for (Documents::const_iterator
           it = context->getDocuments().begin(),
           end = context->getDocuments().end(); it != end; ++it) {
      m_docs.addItem((*it)->getFilename().c_str());
    }
    m_docs.addItem("---------");
  }

private:
  Box m_vbox;
  ComboBox m_docs;
};

class DeveloperConsoleCommand : public Command {
public:
  DeveloperConsoleCommand();
  ~DeveloperConsoleCommand();

protected:
  void onExecute(Context* context);

  DeveloperConsole* m_devConsole;
};

DeveloperConsoleCommand::DeveloperConsoleCommand()
  : Command("DeveloperConsole",
            "DeveloperConsole",
            CmdUIOnlyFlag)
{
  m_devConsole = NULL;
}

DeveloperConsoleCommand::~DeveloperConsoleCommand()
{
  // delete m_devConsole;
}

void DeveloperConsoleCommand::onExecute(Context* context)
{
  if (!m_devConsole) {
    m_devConsole = new DeveloperConsole();
  }
  else if (m_devConsole->isVisible()) {
    m_devConsole->closeWindow(NULL);
    return;
  }

  m_devConsole->updateDocuments(context);
  m_devConsole->openWindow();
}

Command* CommandFactory::createDeveloperConsoleCommand()
{
  return new DeveloperConsoleCommand;
}

} // namespace app
