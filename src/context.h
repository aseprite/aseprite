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

#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include "base/disable_copying.h"
#include "base/exception.h"
#include "context_flags.h"
#include "context_observer.h"
#include "context_observer_list.h"
#include "documents.h"
#include "settings/settings.h"

#include <vector>

class Command;
class Document;
class Params;

class CommandPreconditionException : public base::Exception
{
public:
  CommandPreconditionException() throw()
  : base::Exception("Cannot execute the command because its pre-conditions are false.") { }
};

class Context
{
public:
  virtual ~Context();

  virtual bool isUiAvailable() const     { return false; }
  virtual bool isRecordingMacro() const  { return false; }
  virtual bool isExecutingMacro() const  { return false; }
  virtual bool isExecutingScript() const { return false; }

  ISettings* getSettings() { return m_settings; }

  const Documents& getDocuments() const;
  Document* getFirstDocument() const;
  Document* getNextDocument(Document* document) const;

  bool checkFlags(uint32_t flags) const { return m_flags.check(flags); }
  void updateFlags() { m_flags.update(this); }

  // Appends the document to the context's documents' list.
  void addDocument(Document* document);
  void removeDocument(Document* document);
  void sendDocumentToTop(Document* document);

  Document* getActiveDocument() const;
  void setActiveDocument(Document* document);

  virtual void executeCommand(Command* command, Params* params = NULL);

  void addObserver(ContextObserver* observer);
  void removeObserver(ContextObserver* observer);

protected:

  // The "settings" are deleted automatically in the ~Context destructor
  Context(ISettings* settings);

  virtual void onAddDocument(Document* document);
  virtual void onRemoveDocument(Document* document);
  virtual void onSetActiveDocument(Document* document);

private:

  // Without default constructor
  Context();

  // List of all documents.
  Documents m_documents;

  // Active/selected document to operate.
  Document* m_activeDocument;

  // Settings in this context.
  ISettings* m_settings;

  // Last updated flags.
  ContextFlags m_flags;

  ContextObserverList m_observers;

  DISABLE_COPYING(Context);

};

#endif
