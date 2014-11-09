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

#include "app/context.h"

#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/document.h"
#include "app/document_location.h"
#include "app/settings/settings.h"

#include <algorithm>

namespace app {

Context::Context()
  : m_settings(NULL)
{
}

Context::Context(ISettings* settings)
  : m_settings(settings)
{
  setSettings(settings);
}

Context::~Context()
{
  delete m_settings;
  m_settings = NULL;
}

void Context::sendDocumentToTop(doc::Document* document)
{
  ASSERT(document != NULL);

  documents().move(document, 0);
}

app::Document* Context::activeDocument() const
{
  return static_cast<app::Document*>(doc::Context::activeDocument());
}

DocumentLocation Context::activeLocation() const
{
  DocumentLocation location;
  onGetActiveLocation(&location);
  ASSERT(location.document() == doc::Context::activeDocument());
  return location;
}

void Context::executeCommand(const char* commandName)
{
  Command* cmd = CommandsModule::instance()->getCommandByName(commandName);
  if (cmd)
    executeCommand(cmd);
  else
    throw std::runtime_error("Invalid command name");
}

void Context::executeCommand(Command* command, Params* params)
{
  Console console;

  ASSERT(command != NULL);

  PRINTF("Executing '%s' command.\n", command->short_name());
  BeforeCommandExecution(command);

  try {
    m_flags.update(this);

    if (params)
      command->loadParams(params);

    if (command->isEnabled(this)) {
      command->execute(this);

      AfterCommandExecution(command);
    }
  }
  catch (base::Exception& e) {
    PRINTF("Exception caught executing '%s' command\n%s\n",
           command->short_name(), e.what());

    Console::showException(e);
  }
  catch (std::exception& e) {
    PRINTF("std::exception caught executing '%s' command\n%s\n",
           command->short_name(), e.what());

    console.printf("An error ocurred executing the command.\n\nDetails:\n%s", e.what());
  }
#ifndef DEBUGMODE
  catch (...) {
    PRINTF("unknown exception executing '%s' command\n",
           command->short_name());

    console.printf("An unknown error ocurred executing the command.\n"
                   "Please save your work, close the program, try it\n"
                   "again, and report this bug.\n\n"
                   "Details: Unknown exception caught. This can be bad\n"
                   "memory access, divison by zero, etc.");
  }
#endif
}

void Context::onCreateDocument(doc::CreateDocumentArgs* args)
{
  args->setDocument(new app::Document(NULL));
}

void Context::onGetActiveLocation(DocumentLocation* location) const
{
  // Without active location
}

} // namespace app
