// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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

    app::ISettings* settings() { return m_settings; }

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
