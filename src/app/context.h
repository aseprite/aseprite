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

#ifndef APP_CONTEXT_H_INCLUDED
#define APP_CONTEXT_H_INCLUDED
#pragma once

#include "app/context_flags.h"
#include "base/disable_copying.h"
#include "base/exception.h"
#include "base/signal.h"
#include "doc/context.h"

#include <vector>

namespace app {
  class Command;
  class Document;
  class DocumentLocation;
  class ISettings;
  class Params;

  class CommandPreconditionException : public base::Exception {
  public:
    CommandPreconditionException() throw()
    : base::Exception("Cannot execute the command because its pre-conditions are false.") { }
  };

  class Context : public doc::Context {
  public:
    Context();
    // The "settings" are deleted automatically in the ~Context destructor
    Context(ISettings* settings);
    virtual ~Context();

    virtual bool isUiAvailable() const     { return false; }
    virtual bool isRecordingMacro() const  { return false; }
    virtual bool isExecutingMacro() const  { return false; }
    virtual bool isExecutingScript() const { return false; }

    ISettings* settings() { return m_settings; }

    bool checkFlags(uint32_t flags) const { return m_flags.check(flags); }
    void updateFlags() { m_flags.update(this); }

    void sendDocumentToTop(doc::Document* document);

    app::Document* activeDocument() const;
    DocumentLocation activeLocation() const;

    void executeCommand(const char* commandName);
    virtual void executeCommand(Command* command, Params* params = NULL);

    Signal1<void, Command*> BeforeCommandExecution;
    Signal1<void, Command*> AfterCommandExecution;

  protected:
    virtual void onCreateDocument(doc::CreateDocumentArgs* args) override;
    virtual void onGetActiveLocation(DocumentLocation* location) const;

  private:
    // Settings in this context.
    ISettings* m_settings;

    // Last updated flags.
    ContextFlags m_flags;

    DISABLE_COPYING(Context);
  };

} // namespace app

#endif
