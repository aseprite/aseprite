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
#include "context.h"
#include "document.h"
#include "documents.h"
#include "gui/box.h"
#include "gui/button.h"
#include "gui/combobox.h"
#include "gui/frame.h"

class DeveloperConsole : public Frame
{
public:
  DeveloperConsole()
    : Frame(false, "Developer Console")
    , m_vbox(JI_VERTICAL)
  {
    m_vbox.addChild(&m_docs);
    addChild(&m_vbox);

    remap_window();
    center_window();
  }

  void updateDocuments(Context* context)
  {
    m_docs.removeAllItems();
    m_docs.addItem("Documents");
    for (Documents::const_iterator
	   it = context->getDocuments().begin(),
	   end = context->getDocuments().end(); it != end; ++it) {
      m_docs.addItem((*it)->getFilename());
    }
    m_docs.addItem("---------");
  }

private:
  Box m_vbox;
  ComboBox m_docs;
};

class DeveloperConsoleCommand : public Command
{
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
  m_devConsole->open_window_bg();
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createDeveloperConsoleCommand()
{
  return new DeveloperConsoleCommand;
}
