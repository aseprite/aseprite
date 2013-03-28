/* ASEPRITE
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

#include "config.h"

#include "context.h"

#include "commands/command.h"
#include "console.h"
#include "document.h"
#include "document_location.h"

#include <algorithm>

Context::Context(ISettings* settings)
  : m_settings(settings)
{
}

Context::~Context()
{
  // The context must be empty at this point.
  ASSERT(m_documents.empty());

  delete m_settings;
}

const Documents& Context::getDocuments() const
{
  return m_documents;
}

void Context::addDocument(Document* document)
{
  ASSERT(document != NULL);

  m_documents.addDocument(document);

  // Generate onAddDocument event
  onAddDocument(document);
}

void Context::removeDocument(Document* document)
{
  ASSERT(document != NULL);

  // Remove the item from the documents list.
  m_documents.removeDocument(document);

  // generate onRemoveDocument event
  onRemoveDocument(document);
}

void Context::sendDocumentToTop(Document* document)
{
  ASSERT(document != NULL);

  m_documents.moveDocument(document, 0);
}

Document* Context::getActiveDocument() const
{
  DocumentLocation location;
  onGetActiveLocation(&location);
  return location.document();
}

DocumentLocation Context::getActiveLocation() const
{
  DocumentLocation location;
  onGetActiveLocation(&location);
  return location;
}

void Context::executeCommand(Command* command, Params* params)
{
  Console console;

  ASSERT(command != NULL);

  PRINTF("Executing '%s' command.\n", command->short_name());
  m_observers.notifyCommandBeforeExecution(this);

  try {
    m_flags.update(this);

    if (params)
      command->loadParams(params);

    if (command->isEnabled(this)) {
      command->execute(this);

      m_observers.notifyCommandAfterExecution(this);
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

void Context::addObserver(ContextObserver* observer)
{
  m_observers.addObserver(observer);
}

void Context::removeObserver(ContextObserver* observer)
{
  m_observers.removeObserver(observer);
}

void Context::onAddDocument(Document* document)
{
  // do nothing
}

void Context::onRemoveDocument(Document* document)
{
  // do nothing
}
