// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/document.h"

#include <algorithm>
#include <stdexcept>

namespace app {

Context::Context()
{
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

bool Context::hasModifiedDocuments() const
{
  for (auto doc : documents())
    if (static_cast<app::Document*>(doc)->isModified())
      return true;
  return false;
}

void Context::executeCommand(const char* commandName)
{
  Command* cmd = CommandsModule::instance()->getCommandByName(commandName);
  if (cmd)
    executeCommand(cmd);
  else
    throw std::runtime_error("Invalid command name");
}

void Context::executeCommand(Command* command, const Params& params)
{
  Console console;

  ASSERT(command != NULL);

  LOG("Context: Executing command '%s'...\n", command->id().c_str());
  try {
    m_flags.update(this);

    command->loadParams(params);

    CommandExecutionEvent ev(command);
    BeforeCommandExecution(ev);

    if (ev.isCanceled()) {
      LOG("Context: '%s' was canceled/simulated.\n", command->id().c_str());
    }
    else if (command->isEnabled(this)) {
      command->execute(this);
      LOG("Context: '%s' executed successfully\n", command->id().c_str());
    }
    else {
      LOG("Context: '%s' is disabled\n", command->id().c_str());
    }

    AfterCommandExecution(ev);

    // TODO move this code to another place (e.g. a Workplace/Tabs widget)
    if (isUIAvailable())
      app_rebuild_documents_tabs();
  }
  catch (base::Exception& e) {
    LOG("Context: Exception caught executing '%s' command\n%s\n",
        command->id().c_str(), e.what());

    Console::showException(e);
  }
  catch (std::exception& e) {
    LOG("Context: std::exception caught executing '%s' command\n%s\n",
        command->id().c_str(), e.what());

    console.printf("An error ocurred executing the command.\n\nDetails:\n%s", e.what());
  }
#ifdef NDEBUG
  catch (...) {
    LOG("Context: Unknown exception executing '%s' command\n",
        command->id().c_str());

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

} // namespace app
