// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/doc.h"
#include "app/site.h"
#include "doc/layer.h"

#include <algorithm>
#include <stdexcept>

namespace app {

Context::Context()
  : m_docs(this)
  , m_lastSelectedDoc(nullptr)
  , m_transaction(nullptr)
{
  m_docs.add_observer(this);
}

Context::~Context()
{
  m_docs.remove_observer(this);
}

void Context::sendDocumentToTop(Doc* document)
{
  ASSERT(document != NULL);

  documents().move(document, 0);
}

Site Context::activeSite() const
{
  Site site;
  onGetActiveSite(&site);
  return site;
}

Doc* Context::activeDocument() const
{
  Site site;
  onGetActiveSite(&site);
  return site.document();
}

void Context::setActiveDocument(Doc* document)
{
  onSetActiveDocument(document);
}

bool Context::hasModifiedDocuments() const
{
  for (auto doc : documents())
    if (doc->isModified())
      return true;
  return false;
}

void Context::notifyActiveSiteChanged()
{
  Site site = activeSite();
  notify_observers<const Site&>(&ContextObserver::onActiveSiteChange, site);
}

void Context::executeCommand(const char* commandName)
{
  Command* cmd = Commands::instance()->byId(commandName);
  if (cmd)
    executeCommand(cmd);
  else
    throw std::runtime_error("Invalid command name");
}

void Context::executeCommand(Command* command, const Params& params)
{
  Console console;

  ASSERT(command != NULL);
  if (command == NULL)
    return;

  LOG(VERBOSE) << "CTXT: Executing command " << command->id() << "\n";
  try {
    m_flags.update(this);

    ASSERT(!command->needsParams() || !params.empty());

    command->loadParams(params);

    CommandExecutionEvent ev(command);
    BeforeCommandExecution(ev);

    if (ev.isCanceled()) {
      LOG(VERBOSE) << "CTXT: Command " << command->id() << " was canceled/simulated.\n";
    }
    else if (command->isEnabled(this)) {
      command->execute(this);
      LOG(VERBOSE) << "CTXT: Command " << command->id() << " executed successfully\n";
    }
    else {
      LOG(VERBOSE) << "CTXT: Command " << command->id() << " is disabled\n";
    }

    AfterCommandExecution(ev);

    // TODO move this code to another place (e.g. a Workplace/Tabs widget)
    if (isUIAvailable())
      app_rebuild_documents_tabs();
  }
  catch (base::Exception& e) {
    LOG(ERROR) << "CTXT: Exception caught executing " << command->id() << " command\n"
               << e.what() << "\n";

    Console::showException(e);
  }
  catch (std::exception& e) {
    LOG(ERROR) << "CTXT: std::exception caught executing " << command->id() << " command\n"
               << e.what() << "\n";

    console.printf("An error ocurred executing the command.\n\nDetails:\n%s", e.what());
  }
#ifdef NDEBUG
  catch (...) {
    LOG(ERROR) << "CTXT: Unknown exception executing " << command->id() << " command\n";

    console.printf("An unknown error ocurred executing the command.\n"
                   "Please save your work, close the program, try it\n"
                   "again, and report this bug.\n\n"
                   "Details: Unknown exception caught. This can be bad\n"
                   "memory access, divison by zero, etc.");
  }
#endif
}

void Context::onAddDocument(Doc* doc)
{
  m_lastSelectedDoc = doc;
}

void Context::onRemoveDocument(Doc* doc)
{
  if (doc == m_lastSelectedDoc)
    m_lastSelectedDoc = nullptr;
}

void Context::onGetActiveSite(Site* site) const
{
  // Default/dummy site (maybe for batch/command line mode)
  if (Doc* doc = m_lastSelectedDoc) {
    site->document(doc);
    site->sprite(doc->sprite());
    site->layer(doc->sprite()->root()->firstLayer());
    site->frame(0);
  }
}

void Context::onSetActiveDocument(Doc* doc)
{
  m_lastSelectedDoc = doc;
}

void Context::setTransaction(Transaction* transaction)
{
  if (transaction) {
    ASSERT(!m_transaction);
    m_transaction = transaction;
  }
  else {
    ASSERT(m_transaction);
    m_transaction = nullptr;
  }
}

} // namespace app
